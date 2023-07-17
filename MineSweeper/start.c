#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>


//Struct that represents each slot on the minesweeper board
typedef struct slot {
  char visibility;
  char typeOf;
  GtkWidget *btn;  
} slot;
//Default board size
int boardSizeRoot = 10;
//Size of the slot rooted
int sizeOfSlot = 54;

slot **board;
//Used to store indexes that we will free at the end of the program
int* **indexFree;


//Styles for image nums 1-8, this is used for better accessibility
char numStyles[][20] = {"btnStyleOne", "btnStyleTwo", "btnStyleThree", "btnStyleFour", "btnStyleFive", "btnStyleSix", "btnStyleSeven", "btnStyleEight"};

//Amount of flags we can use
int flags;
//Counter for to check
int clicksTillWin;
//Label for amount of flags left
GtkWidget *flagsLeft;
//Smile Reset Button
GtkWidget *smileReset;
//Timer Label
GtkWidget *timer;
//Scroll for the grid, we need it here so we can access it in the zoomScreen function
GtkWidget *scroll;
//We can use this to stop the timer
unsigned int timerId;
//Check if timerId has been stopped
bool timerStopped = false;

//Used to store strings (actually just the flag string lol)
char *buffer;
//Used to store timerText
char *timerText;

//Used to set CSS to a cetrain item
void defineCSS(GtkWidget *item, GtkCssProvider *cssProvider, char *className);
//Used when user clicks on a slot
void buttonClicked(GtkWidget *widget,GdkEventButton *event, gpointer data);
//Used to remove a css class and replace it with another
void changeStyleContext(GtkStyleContext *context, char *remove, char *add);
//Used to set the board
void setBoard(int i, int j);
//Used to check around an element in a matrix without going out of bounds (returns amount of mines around the element)
int checkAround(int i, int j);
//Used to make everything around a clicked zero visible 
void floodFill(int i, int j);
//Called when user clicks on a mine
void gameLost();
//Called when user wins
void gameWon();
//Updates timer label
int updateTimerLabel();
//Function to zoom in/out on the board
void zoomScreen(GtkWidget *widget, GdkEventKey *event, gpointer data);
//Restarts Minesweeper
void restartApp() {char *args[] = {"./start", NULL}; execv(args[0], args);}

//Needed here so we can close it lol
GtkWidget *dialog;
bool destroyedDialog = false;
//Set difficulty 
void setDif(GtkWidget *widget, gpointer data) {
   boardSizeRoot = *((int *)data);
   gtk_widget_destroy(GTK_WIDGET(dialog));
   destroyedDialog = true;
}


void activate(GtkApplication *app, gpointer user_data) {
  //Add CSS file with cssprovider
  GtkCssProvider *cssProvider = gtk_css_provider_new();
  gtk_css_provider_load_from_path(cssProvider, "styles.css", NULL);  

  //DIFFICULTY SET WINDOW
  dialog = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dialog), "Set difficulty");
  gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
  gtk_widget_set_size_request(dialog, 500, 500);
  //Center the dialog
  gtk_widget_set_valign(gtk_dialog_get_content_area(GTK_DIALOG(dialog)), GTK_ALIGN_CENTER);
  gtk_widget_set_halign(gtk_dialog_get_content_area(GTK_DIALOG(dialog)), GTK_ALIGN_CENTER);
  gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
  
  GtkWidget *difGrid = gtk_grid_new();

  //Make difficulty buttons
  GtkWidget *ten = gtk_button_new_with_label("10x10");
  gtk_grid_attach(GTK_GRID(difGrid), ten, 0, 0, 1, 1);
  GtkWidget *eighteen = gtk_button_new_with_label("18x18");
  gtk_grid_attach(GTK_GRID(difGrid), eighteen, 2, 0, 1, 1);
  GtkWidget *twentyfive = gtk_button_new_with_label("25x25");
  gtk_grid_attach(GTK_GRID(difGrid), twentyfive, 1, 1, 1, 1);
  
  //Define the css of the buttons
  defineCSS(dialog, cssProvider, "difScreen");
  defineCSS(ten, cssProvider, "difBtn");
  defineCSS(eighteen, cssProvider, "difBtn");
  defineCSS(twentyfive, cssProvider, "difBtn");
  
  //Set the signals
  int difA = 10, difB = 18, difC = 25;
  g_signal_connect(ten, "clicked", G_CALLBACK(setDif), &difA);
  g_signal_connect(eighteen, "clicked", G_CALLBACK(setDif), &difB);
  g_signal_connect(twentyfive, "clicked", G_CALLBACK(setDif), &difC);
  
  //Add the grid to the box that exists within the dialog  
  gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), difGrid);
  gtk_widget_show_all(dialog);
  gtk_dialog_run(GTK_DIALOG(dialog));
  
  if (!destroyedDialog) gtk_widget_destroy(dialog);
  
   
  //START OF MINESWEEPER
  FILE *fp = fopen("dif.txt", "r");
  if (fp != NULL) {
    fscanf(fp, "%d", &boardSizeRoot);
    fclose(fp);
  }
  
  //Set all needed global variables
  board = (slot**)malloc(boardSizeRoot*sizeof(slot*));
  for (int i = 0; i < boardSizeRoot; i++) board[i] = (slot*)malloc(boardSizeRoot*sizeof(slot));
  
  indexFree = (int***)malloc(boardSizeRoot*sizeof(int**));
  for (int i = 0; i < boardSizeRoot; i++) {
    indexFree[i] = (int**)malloc(boardSizeRoot*sizeof(int*));
    for (int j = 0; j < boardSizeRoot; j++) {
      indexFree[i][j] = (int*)malloc(sizeof(int*));
    }
  }
  flags = (boardSizeRoot-10)+(boardSizeRoot*boardSizeRoot/10);
  clicksTillWin = boardSizeRoot*boardSizeRoot - flags;
  	
		
  //Create Window
  GtkWidget *window = gtk_application_window_new(app);
  //Center the window
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

  //Define the style you're using and add it to context (CSS)
  defineCSS(window, cssProvider, "windowStyle"); 

  // Create a main vertical box container
  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add(GTK_CONTAINER(window), vbox);
  gtk_widget_set_valign(vbox, GTK_ALIGN_CENTER);
  gtk_widget_set_halign(vbox, GTK_ALIGN_CENTER);
  defineCSS(vbox, cssProvider, "borderPane");

  //Extra Info (Nav bar for the game)
  GtkWidget *navBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start(GTK_BOX(vbox), navBox, FALSE, FALSE, 0); 
  
  //Nav Elements
  
  //The buffer is sized by the length of the number added by 3 for the flag, space, and null terminator (Do not free until application ends)
  //This is the flag counter
  buffer = calloc(((int)log10((double)flags)+1)+3, sizeof(char));
  sprintf(buffer, "🚩 %d", flags);
  flagsLeft = gtk_label_new(buffer);
  gtk_box_pack_start(GTK_BOX(navBox), flagsLeft, FALSE, FALSE, 0);
   
  //Smiley Face reset
  smileReset = gtk_button_new_with_label("");
  gtk_style_context_add_class(gtk_widget_get_style_context(smileReset), "navSmileResetPlaying");
  g_signal_connect(smileReset, "clicked", G_CALLBACK(restartApp), NULL);
  gtk_box_pack_start(GTK_BOX(navBox), smileReset, TRUE, FALSE, 0);
  
  //Timer
  timer = gtk_label_new("00:00");
  timerText = strdup(gtk_label_get_text(GTK_LABEL(timer)));
  gtk_box_pack_end(GTK_BOX(navBox), timer, FALSE, FALSE, 0); 

  //Define CSS for Nav bar elements
  defineCSS(navBox, cssProvider, "navBox");
  defineCSS(flagsLeft, cssProvider, "navFlagsLeft");
  defineCSS(smileReset, cssProvider, "navSmileReset");
  defineCSS(timer, cssProvider, "navTimer");
  
  //Scroll for the grid
  scroll = gtk_scrolled_window_new(NULL, NULL);
  //Size of a slot is 50px including borders, we have the grid which has 7px borders on each side
  int sSize = boardSizeRoot*sizeOfSlot+14;

  gtk_widget_set_size_request(scroll, (sSize>709?709:sSize), (sSize>709?709:sSize)); 
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_end(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);

  //Grid
  GtkWidget *grid = gtk_grid_new();
  gtk_container_add(GTK_CONTAINER(scroll), grid);
  defineCSS(grid, cssProvider, "playGrid");

  //Create Buttons
  bool counter = 0, prev = 1;
  for (int i = 0; i < boardSizeRoot; i++) {
    //Make sure that the counter alternates for rows
    counter = !prev;
    prev = counter;
    for (int j = 0; j < boardSizeRoot; j++) {
      //Set defaults to struct array board
      board[i][j].visibility = 0;
      board[i][j].typeOf = 0;
      board[i][j].btn = gtk_button_new_with_label("");

      //Set size of slot
      gtk_widget_set_size_request(board[i][j].btn, sizeOfSlot, sizeOfSlot);
      //Attach button to grid (j represents left, i represents top)
      gtk_grid_attach(GTK_GRID(grid), board[i][j].btn, j, i, 1, 1);
      
      //Passed into the click event so they know what button it is (Do not free until application ends)
      
      int *index = (int *)malloc(2 * sizeof(int));
      index[0] = i;
      index[1] = j;
      indexFree[i][j] = index;
      
      //Listen to a right click or left click on a slot
      g_signal_connect(board[i][j].btn, "button-press-event", G_CALLBACK(buttonClicked), index);

      //Define button css
      defineCSS(board[i][j].btn, cssProvider, "btnStyle");

      //Add the image class to the buttons
      gtk_style_context_add_class(gtk_widget_get_style_context(board[i][j].btn), counter ? "btnStyle2" : "btnStyle1");
      //Alternate the counter for each column
      counter = !counter;
    }
  }
  
  //Set Default Size of GUI
  gtk_window_set_default_size(GTK_WINDOW(window), 1000, 800);
  //Sets Border of GUI
  gtk_container_set_border_width(GTK_CONTAINER(window), 10);
  //Set Title of GUI
  gtk_window_set_title(GTK_WINDOW(window), "Minesweeper");
  
  //Zoom in/out listener
  g_signal_connect(window, "key-press-event", G_CALLBACK(zoomScreen), NULL); 
  

  //Show Window
  gtk_widget_show_all(window);
  
  //unref Resources that are no longer needed
  g_object_unref(cssProvider);
  
  
} 

int main(int argc, char *argv[])
{
    GtkApplication *app = gtk_application_new("in.minesweeper", G_APPLICATION_NON_UNIQUE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int ret = g_application_run(G_APPLICATION(app), argc, argv);
   
    //Free Resources after application ends
    g_object_unref(app);
    free(buffer);
    free(timerText);
    for (int i = 0; i < boardSizeRoot; i++) {
      for (int j = 0; j < boardSizeRoot; j++) free(indexFree[i][j]);
      free(indexFree[i]);
      free(board[i]);
    }
    free(indexFree);
    free(board);
    
    return ret;
}

//Define CSS function
void defineCSS(GtkWidget *item, GtkCssProvider *cssProvider, char *className) {
  //Set the context to the item
  GtkStyleContext *styleContext = gtk_widget_get_style_context(item);
  //Connect the cssProvider (connected to the css file) to the styleContext
  gtk_style_context_add_provider(styleContext, GTK_STYLE_PROVIDER(cssProvider),
                   GTK_STYLE_PROVIDER_PRIORITY_USER);
  //Connect the class to the context
  gtk_style_context_add_class(styleContext, className);
}


bool gameStart = true;
void buttonClicked(GtkWidget *widget, GdkEventButton *event ,gpointer data) {
  //Data should not be freed until application ends because the same button can be clicked multiple times
  int *index = (int *)data;

  if (gameStart && event->button == GDK_BUTTON_SECONDARY) return;
  if (gameStart) {
    setBoard(index[0], index[1]);
    timerId = g_timeout_add_seconds(1, updateTimerLabel, NULL);
    gameStart = !gameStart;
  }
  
  GtkStyleContext *context = gtk_widget_get_style_context(widget);
  char visible = board[index[0]][index[1]].visibility;
  if (visible == 0 && event->button == GDK_BUTTON_PRIMARY) { 
    //Make the slot visible
    changeStyleContext(context, "btnStyle1", "btnVisible1");
    changeStyleContext(context, "btnStyle2", "btnVisible2");
    
    //Clicks Till Win lowered
    clicksTillWin--;
    
    switch(board[index[0]][index[1]].typeOf) {
      case -1:
      	//prevent a bug
        clicksTillWin++;
        
      	gameLost();
        changeStyleContext(gtk_widget_get_style_context(smileReset), "navSmileResetPlaying", "navSmileResetLose");
      break;
      case 0:
        //prevent a bug
        clicksTillWin++;
        
    	  floodFill(index[0], index[1]);
      break;
      default:
        //numStyles is used here to access the needed num image
        gtk_style_context_add_class(context, numStyles[board[index[0]][index[1]].typeOf - 1]);
    }
    board[index[0]][index[1]].visibility = 1;
    //Win the game
    if (clicksTillWin == 0) gameWon(); 
    
  } else if (visible == 0 && event->button == GDK_BUTTON_SECONDARY) {
    if (flags > 0) {
      //Add a class
      gtk_style_context_add_class(context, "btnStyleFlag");

      board[index[0]][index[1]].visibility = 2;
      sprintf(buffer, "🚩 %d", --flags);
      gtk_label_set_text(GTK_LABEL(flagsLeft), buffer);
    }
  } else if (visible == 2 && event->button == GDK_BUTTON_SECONDARY) {
    //Remove a class
    gtk_style_context_remove_class(context, "btnStyleFlag");

    board[index[0]][index[1]].visibility = 0;
    sprintf(buffer, "🚩 %d", ++flags);
    gtk_label_set_text(GTK_LABEL(flagsLeft), buffer); 
  }
} 

//Shortcut for replacing a class with another
void changeStyleContext(GtkStyleContext *context, char *remove, char *add) {
  if (gtk_style_context_has_class(context, remove)) {
    gtk_style_context_remove_class(context, remove);
    gtk_style_context_add_class(context, add);
  } 
}

/*
 * Sets the board to play the game 
 * (i and j represent a slot that should be set to 0 when playing if you don't want to limit the amount of slots set both i and j to -1) @author (TheDestroyerOfAllWorlds) @ github
 */
void setBoard(int i, int j) {
  srand(time(NULL));
  // resets the board by setting all the values of the matrix to 0
  for (int rows = 0; rows < boardSizeRoot; rows++) {
    for (int columns = 0; columns < boardSizeRoot; columns++) {
      board[rows][columns].typeOf = 0;
    }
  }
  // initilizing the randomness, bombCount
  int randomRow;
  int randomColumn;
  int bombCount = 0;
  // while there's less bombs placed than flags available, a random tile (using a random row or column in the matrix) will be set to -1 if it's an empty slot
  while (bombCount < flags) {
    //This is used to continue this while loop
    bool conLoop = false;
    randomRow = rand() % boardSizeRoot;
    randomColumn = rand() % boardSizeRoot;
    //If the input of i and j wasn't -1 we will ignore placing mines around board[i][j]
    if (i != -1 && j != -1) {
      for (int a = (i == 0 ? 0 : i-1); a < (i+1 == boardSizeRoot ? i+1 : i+2); a++) {
        for (int b = (j == 0 ? 0 : j-1); b < (j+1 == boardSizeRoot ? j+1 : j+2); b++) {
          if (a == randomRow && b == randomColumn) {
            conLoop = true;
            break;
          }
        }
        if (conLoop) break;
      }
      if (conLoop) continue;
    }
    if (board[randomRow][randomColumn].typeOf == 0) {
      board[randomRow][randomColumn].typeOf = -1;
      bombCount++;
    }
  }
  // uses the checkAround function for every tile now that there's bombs.
  for (int rows = 0; rows < boardSizeRoot; rows++) {
    for (int columns = 0; columns < boardSizeRoot; columns++) {
      if (board[rows][columns].typeOf == 0) {
        board[rows][columns].typeOf = checkAround(rows, columns);
      }
    }
  }
}
//Checks around a certain element and returns amount of mines around the element
int checkAround(int i, int j) {
  int counter = 0;
  for (int a = (i == 0 ? 0 : i-1); a < (i+1 == boardSizeRoot ? i+1 : i+2); a++) {
    for (int b = (j == 0 ? 0 : j-1); b < (j+1 == boardSizeRoot ? j+1 : j+2); b++) {
      if (board[a][b].typeOf == -1) counter++;
    }
  }
  if (board[i][j].typeOf == -1) counter -= 1;
  return counter;
}

//This makes all squares around a 0 visible. Make sure the i and j are always values where the slot's typeOf is set to 0
void floodFill(int i, int j) {
  for (int a = (i == 0 ? 0 : i-1); a < (i+1 == boardSizeRoot ? i+1 : i+2); a++) {
    for (int b = (j == 0 ? 0 : j-1); b < (j+1 == boardSizeRoot ? j+1 : j+2); b++) {
      //Lower clicks till win
      if (board[a][b].visibility == 0) clicksTillWin--;
      
      if (board[a][b].typeOf == 0 && board[a][b].visibility == 0) {
        board[a][b].visibility = 1;
        floodFill(a, b);
      }
      //Ignore visible and flagged squares
      if (board[a][b].visibility == 2) continue;
      board[a][b].visibility = 1;
      GtkStyleContext *context = gtk_widget_get_style_context(board[a][b].btn);
      
    	changeStyleContext(context, "btnStyle1", "btnVisible1");
    	changeStyleContext(context, "btnStyle2", "btnVisible2");
        
      //We use numStyles here to access image nums
      gtk_style_context_add_class(context, numStyles[board[a][b].typeOf - 1]);
    }
  }
}
void gameLost() {
  if (!timerStopped) g_source_remove(timerId);
  timerStopped = true;
  for (int i = 0; i < boardSizeRoot; i++) {
    for (int j = 0; j < boardSizeRoot; j++) {
      if (board[i][j].typeOf == -1 && board[i][j].visibility != 2) {
        GtkStyleContext *context = gtk_widget_get_style_context(board[i][j].btn);
        changeStyleContext(context, "btnStyle1", "btnVisible1");
        changeStyleContext(context, "btnStyle2", "btnVisible2");
        gtk_style_context_add_class(context, "btnStyleMine");
      }
      board[i][j].visibility = 1;
    }
  }
}

void gameWon() {
  if (!timerStopped) g_source_remove(timerId);
  timerStopped = true;
  changeStyleContext(gtk_widget_get_style_context(smileReset), "navSmileResetPlaying", "navSmileResetWin");
  for (int i = 0; i < boardSizeRoot; i++) for (int j = 0; j < boardSizeRoot; j++) board[i][j].visibility = 1; 
}


int updateTimerLabel(gpointer data) {
   if (strcmp(timerText, "99:59") == 0) {
     timerStopped = true;
     return G_SOURCE_REMOVE;
   }
   //We basically overflow numbers, if a number overflows just add 1 to the number left of it.
   for (int i = strlen(timerText)-1; i >= 0; i--) {
     //2 is when we reach the ":"
     if (i == 2) continue;
     //When i is 3 we reached 00:-->0<--0 that zero and since we want accurate time, seconds should reset at 59
     if (timerText[i] != (i != 3 ? '9' : '5')) {
     	timerText[i]++;
     	break;
     } else {
       timerText[i] = '0';
     }
   }
   //Set the label
   gtk_label_set_text(GTK_LABEL(timer), timerText);
   
   //Continue calling this function
   return G_SOURCE_CONTINUE;
}

void zoomScreen(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    //Used to check if we have to change the board for this function call
    bool changing = 0;
    
    //If we type the down key make each slot smaller
    if (event->keyval == GDK_KEY_Down) {
    	sizeOfSlot -= 10;
    	changing = 1;
    //If we type the up key make each slot bigger
    } else if (event->keyval == GDK_KEY_Up) {
        sizeOfSlot += 10;
        changing = 1;
    }
    //24 because the min width in the css in 20 and the slots have 2px border on each side
    if (sizeOfSlot < 24) sizeOfSlot = 24;
    
    //This is a manually set max :0
    if (sizeOfSlot > 204) sizeOfSlot = 204;
    if (!changing) return;
    
    //set size request for all slots
    for (int i = 0; i < boardSizeRoot; i++) for (int j = 0; j < boardSizeRoot; j++) gtk_widget_set_size_request(board[i][j].btn, sizeOfSlot, sizeOfSlot);
    
    //Set the scroll screen to fit the adjusted board
    int setScrollSize = sizeOfSlot*boardSizeRoot+14;
    gtk_widget_set_size_request(scroll, (setScrollSize>709?709:setScrollSize),(setScrollSize>709?709:setScrollSize));
}
