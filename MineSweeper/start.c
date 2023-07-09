#include <gtk/gtk.h>
#include <gdk/gdk.h>
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

//Used to set CSS to a cetrain item
void defineCSS(GtkWidget *item, GtkCssProvider *cssProvider, char *className);
void buttonClicked(GtkWidget *widget,GdkEventButton *event, gpointer data);
void changeStyleContext(GtkStyleContext *context, char *remove, char *add);
void setBoard(int i, int j);
int checkAround(int i, int j);
void floodFill(int i, int j);


static void activate(GtkApplication *app, gpointer user_data) {
  //Create Window
  GtkWidget *window = gtk_application_window_new(app);
  
  //Add CSS file with cssprovider
  GtkCssProvider *cssProvider = gtk_css_provider_new();
  gtk_css_provider_load_from_path(cssProvider, "styles.css", NULL);  

  //Define the style you're using and add it to context (CSS)
  defineCSS(window, cssProvider, "windowStyle"); 

  // Create a main vertical box container
  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add(GTK_CONTAINER(window), vbox);

  //Grid
  GtkWidget *grid = gtk_grid_new();
  gtk_box_pack_start(GTK_BOX(vbox), grid, TRUE, TRUE, 0); 
  gtk_widget_set_halign(grid, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(grid, GTK_ALIGN_CENTER);

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
      
      //Listen to click for button
      int *index = calloc(2, sizeof(int));
      index[0] = i;
      index[1] = j;

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
  
  //Free Resources when no longer needed
  g_object_unref(cssProvider);
} 

int main(int argc, char *argv[])
{
    GtkApplication *app = gtk_application_new("in.minesweeper", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    int ret = g_application_run(G_APPLICATION(app), argc, argv);
    
    g_object_ref(app);
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
  //Data should not be freed because same button can be clicked multiple times
  int *index = (int *)data;
  
  if (gameStart) {
    setBoard(index[0], index[1]);
    gameStart = !gameStart;
  }
  
  GtkStyleContext *context = gtk_widget_get_style_context(widget);
  char visible = board[index[0]][index[1]].visibility;
  if (visible == 0 && event->button == GDK_BUTTON_PRIMARY) {
    switch(board[index[0]][index[1]].typeOf) {
      case -1:
      	changeStyleContext(context, "btnStyle1", "btnStyleMine1");
    	changeStyleContext(context, "btnStyle2", "btnStyleMine2");
      break;
      case 0:
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
  } else if (visible == 0 && event->button == GDK_BUTTON_SECONDARY) {
    changeStyleContext(context, "btnStyle1", "btnStyleFlag1");
    changeStyleContext(context, "btnStyle2", "btnStyleFlag2");
    board[index[0]][index[1]].visibility = 2;
  } else if (visible == 2 && event->button == GDK_BUTTON_SECONDARY) {
    changeStyleContext(context, "btnStyleFlag1", "btnStyle1");
    changeStyleContext(context, "btnStyleFlag2", "btnStyle2");
    board[index[0]][index[1]].visibility = 0;
  } 
} 
//Shortcut for changing class
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
  // initilizing the randomness, bombCount, and continue loop
  int randomRow;
  int randomColumn;
  int bombCount = 0;
  // while there's less than boardSizeRoot bombs, a random tile (using a random row or column in the matrix) will be set to -1 if it's an empty slot
  while (bombCount < boardSizeRoot) {
    bool conLoop = false;
    randomRow = rand() % boardSizeRoot;
    randomColumn = rand() % boardSizeRoot;
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
      if (board[a][b].typeOf == 0 && board[a][b].visibility != 1) {
        board[a][b].visibility = 1;
        floodFill(a, b);
      }
      GtkStyleContext *context = gtk_widget_get_style_context(board[a][b].btn);
      
      switch(board[a][b].typeOf) {
        case -1:
      	  changeStyleContext(context, "btnStyle1", "btnStyleMine1");
    	  changeStyleContext(context, "btnStyle2", "btnStyleMine2");
        break;
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
