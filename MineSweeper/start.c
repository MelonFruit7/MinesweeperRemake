#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>


//Struct that represents each slot on the minesweeper board
typedef struct slot {
  char visibility;
  char typeOf;
  GtkWidget *btn;  
} slot;
#define boardSizeRoot 10
slot board[boardSizeRoot][boardSizeRoot];
//Used to store indexes that we will free at the end of the program
int* indexFree[boardSizeRoot][boardSizeRoot];

//Amount of flags we can use
int flags = (boardSizeRoot-10)+(boardSizeRoot*boardSizeRoot/10);
//Counter for to check
int clicksTillWin = boardSizeRoot*boardSizeRoot - ((boardSizeRoot-10)+(boardSizeRoot*boardSizeRoot/10));
//Label for amount of flags left
GtkWidget *flagsLeft;
//Smile Reset Button
GtkWidget *smileReset;
//Timer Label
GtkWidget *timer;
//We can use this to stop the timer
unsigned int timerId;

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
//Restarts Minesweeper
void restartApp() { system("killall start; ./start"); }


static void activate(GtkApplication *app, gpointer user_data) {
  //Create Window
  GtkWidget *window = gtk_application_window_new(app);
  //Center the window
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

  //Add CSS file with cssprovider
  GtkCssProvider *cssProvider = gtk_css_provider_new();
  gtk_css_provider_load_from_path(cssProvider, "styles.css", NULL);  

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

  //Grid
  GtkWidget *grid = gtk_grid_new();
  gtk_box_pack_end(GTK_BOX(vbox), grid, TRUE, TRUE, 0); 
  defineCSS(grid, cssProvider, "playGrid");

  //Create Buttons
  int counter = 0, prev = 1;
  for (int i = 0; i < boardSizeRoot; i++) {
    //Make sure that the counter alternates for rows
    counter = !prev;
    prev = counter;
    for (int j = 0; j < boardSizeRoot; j++) {
      //Set defaults to struct array board
      board[i][j].visibility = 0;
      board[i][j].typeOf = 0;
      board[i][j].btn = gtk_button_new_with_label("");
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


  //Show Window
  gtk_widget_show_all(window);
  
  //unref Resources that are no longer needed
  g_object_unref(cssProvider);
} 

int main(int argc, char *argv[])
{
    GtkApplication *app = gtk_application_new("in.minesweeper", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int ret = g_application_run(G_APPLICATION(app), argc, argv);

    //Free Resources after application ends
    g_object_unref(app);
    free(buffer);
    free(timerText);
    for (int i = 0; i < boardSizeRoot; i++) for (int j = 0; j < boardSizeRoot; j++) free(indexFree[i][j]);
    
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
        
    	  changeStyleContext(context, "btnStyle1", "btnVisible1");
    	  changeStyleContext(context, "btnStyle2", "btnVisible2");
    	  floodFill(index[0], index[1]);
      break;
      case 1:
        changeStyleContext(context, "btnStyle1", "btnStyleOne1");
    	  changeStyleContext(context, "btnStyle2", "btnStyleOne2");
      break;
      case 2:
        changeStyleContext(context, "btnStyle1", "btnStyleTwo1");
    	  changeStyleContext(context, "btnStyle2", "btnStyleTwo2");
      break;
      case 3:
        changeStyleContext(context, "btnStyle1", "btnStyleThree1");
    	  changeStyleContext(context, "btnStyle2", "btnStyleThree2");
      break;
      case 4:
        changeStyleContext(context, "btnStyle1", "btnStyleFour1");
    	  changeStyleContext(context, "btnStyle2", "btnStyleFour2");
      break;
      case 5:
        changeStyleContext(context, "btnStyle1", "btnStyleFive1");
    	  changeStyleContext(context, "btnStyle2", "btnStyleFive2");
      break;
      case 6:
        changeStyleContext(context, "btnStyle1", "btnStyleSix1");
    	  changeStyleContext(context, "btnStyle2", "btnStyleSix2");
      break;
      case 7:
        changeStyleContext(context, "btnStyle1", "btnStyleSeven1");
    	  changeStyleContext(context, "btnStyle2", "btnStyleSeven2");
      break;
      case 8:
        changeStyleContext(context, "btnStyle1", "btnStyleEight1");
    	  changeStyleContext(context, "btnStyle2", "btnStyleEight2");
      break;
    }
    board[index[0]][index[1]].visibility = 1;
    //Win the game
    if (clicksTillWin == 0) gameWon(); 
    
  } else if (visible == 0 && event->button == GDK_BUTTON_SECONDARY) {
    if (flags > 0) {
      changeStyleContext(context, "btnStyle1", "btnStyleFlag1");
      changeStyleContext(context, "btnStyle2", "btnStyleFlag2");
      board[index[0]][index[1]].visibility = 2;
      sprintf(buffer, "🚩 %d", --flags);
      gtk_label_set_text(GTK_LABEL(flagsLeft), buffer);
    }
  } else if (visible == 2 && event->button == GDK_BUTTON_SECONDARY) {
    changeStyleContext(context, "btnStyleFlag1", "btnStyle1");
    changeStyleContext(context, "btnStyleFlag2", "btnStyle2");
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
      
      switch(board[a][b].typeOf) {
        //-1 is redundant since we don't want to make a mine visible lol
        case 0:
    	  changeStyleContext(context, "btnStyle1", "btnVisible1");
    	  changeStyleContext(context, "btnStyle2", "btnVisible2");
        break;
        case 1:
          changeStyleContext(context, "btnStyle1", "btnStyleOne1");
    	  changeStyleContext(context, "btnStyle2", "btnStyleOne2");
        break;
        case 2:
          changeStyleContext(context, "btnStyle1", "btnStyleTwo1");
    	  changeStyleContext(context, "btnStyle2", "btnStyleTwo2");
        break;
        case 3:
          changeStyleContext(context, "btnStyle1", "btnStyleThree1");
    	  changeStyleContext(context, "btnStyle2", "btnStyleThree2");
        break;
        case 4:
          changeStyleContext(context, "btnStyle1", "btnStyleFour1");
    	  changeStyleContext(context, "btnStyle2", "btnStyleFour2");
        break;
        case 5:
          changeStyleContext(context, "btnStyle1", "btnStyleFive1");
    	  changeStyleContext(context, "btnStyle2", "btnStyleFive2");
        break;
        case 6:
          changeStyleContext(context, "btnStyle1", "btnStyleSix1");
    	  changeStyleContext(context, "btnStyle2", "btnStyleSix2");
        break;
        case 7:
          changeStyleContext(context, "btnStyle1", "btnStyleSeven1");
    	  changeStyleContext(context, "btnStyle2", "btnStyleSeven2");
        break;
        case 8:
          changeStyleContext(context, "btnStyle1", "btnStyleEight1");
    	  changeStyleContext(context, "btnStyle2", "btnStyleEight2");
      	break;
      }
    }
  }
}
void gameLost() {
  g_source_remove(timerId);
  for (int i = 0; i < boardSizeRoot; i++) {
    for (int j = 0; j < boardSizeRoot; j++) {
      board[i][j].visibility = 1;
      if (board[i][j].typeOf == -1) {
        GtkStyleContext *context = gtk_widget_get_style_context(board[i][j].btn);
        changeStyleContext(context, "btnStyle1", "btnStyleMine1");
        changeStyleContext(context, "btnStyle2", "btnStyleMine2");
      } 
    }
  }
}
void gameWon() {
  g_source_remove(timerId);
  changeStyleContext(gtk_widget_get_style_context(smileReset), "navSmileResetPlaying", "navSmileResetWin");
  for (int i = 0; i < boardSizeRoot; i++) for (int j = 0; j < boardSizeRoot; j++) board[i][j].visibility = 1;
    
  
}
int updateTimerLabel(gpointer data) {
   for (int i = strlen(timerText)-1; i > 0; i--) {
     if (i == 2) continue;
     if (timerText[i] != (i != 3 ? '9' : '5')) {
     	timerText[i]++;
     	break;
     } else {
     	timerText[i] = '0';
     }
   }
   gtk_label_set_text(GTK_LABEL(timer), timerText);
   return G_SOURCE_CONTINUE;
}
