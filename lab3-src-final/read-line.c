/*
 * CS252: Systems Programming
 * Purdue University
 * Example that shows how to read one line with simple editing
 * using raw terminal.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

#define MAX_BUFFER_LINE 2048

extern void tty_raw_mode(void);

// Buffer where line is stored
int line_length;
int cur_loc;
char line_buffer[MAX_BUFFER_LINE];

// Simple history array
// This history does not change. 
// Yours have to be updated.
int history_index = 0;
/*char * history [] = {
  "ls -al | grep x", 
  "ps -e",
  "cat read-line-example.c",
  "vi hello.c",
  "make",
  "ls -al | grep xxx | grep yyy"
};*/
char * history[32];
//int history_length = sizeof(history)/sizeof(char *);
int history_length = 0;
void read_line_print_usage()
{
  char * usage = "\n"
    " ctrl-?       Print usage\n"
    " Backspace    Deletes last character\n"
    " up arrow     See last command in the history\n";

  write(1, usage, strlen(usage));
}

/* 
 * Input a line with some basic editing.
 */
char * read_line() {

  // Set terminal in raw mode
  tty_raw_mode();

  line_length = 0;
  cur_loc = 0;
  // Read one line until enter is typed
  while (1) {

    // Read one character in raw mode.
    char ch;
    char ct;

    read(0, &ch, 1);

    if (ch>=32) {
      // It is a printable character.

      //Insert middle 
      if(cur_loc < line_length) {
        ct = line_buffer[cur_loc];
	line_buffer[cur_loc] = ch;
	write(1, &ch, 1);
	for(int i=cur_loc+1; i< line_length+1; i++){
	  ch = ct;
	  ct = line_buffer[i];
	  line_buffer[i] = ch;
	  write(1, &ch, 1);
	}
	for(int i=cur_loc+1; i< line_length+1; i++){
	  ch = 8;
	  write(1, &ch, 1);
	}
	cur_loc++;
	line_length++;
      }

      else{
      // Do echo
      write(1,&ch,1);

      // If max number of character reached return.
      if (line_length==MAX_BUFFER_LINE-2) break;

      // add char to buffer.
      line_buffer[line_length]=ch;
      line_length++;
      cur_loc++;
      }
    }
    else if (ch==10) {
      // <Enter> was typed. Return line

      // Print newline
      write(1,&ch,1);

      break;
    }
    else if (ch == 31) {
      // ctrl-?
      read_line_print_usage();
      line_buffer[0]=0;
      break;
    }
    else if (ch == 1) {
	//Home
	for(int i=cur_loc; i>0; i--){
	  ch = 8;
	  write(1, &ch, 1);
	}
	cur_loc = 0;
    }
    else if (ch == 5) {
	//End
	for(int i = cur_loc; i<line_length; i++){
	  ch = 27;
 	  write(1, &ch, 1);
	  ch = 91;
	  write(1, &ch, 1);
	  ch = 67;
	  write(1, &ch, 1);
       }
       cur_loc = line_length;
    }
    else if (ch == 4) {
	//Delete
	if(cur_loc < line_length-1){
	  for(int i=cur_loc; i<line_length-1; i++){
	    ch = line_buffer[i+1];
	    write(1, &ch, 1);
	    line_buffer[i] = line_buffer[i+1];
          }
          ch = ' ';
	  write(1, &ch, 1);
          for(int i= cur_loc; i<line_length; i++){
	    ch = 8;
	    write(1, &ch, 1);
	  }
	  line_length--;
	
	}
	else if(cur_loc == line_length-1){
	ch = ' ';
	write(1, &ch, 1);
	ch = 8;
	write(1, &ch ,1);
	write(1, &ch, 1);
	line_length--;
	cur_loc--;
	}
    }
    else if (ch == 8) {
      // <backspace> was typed. Remove previous character read.
      if(cur_loc<line_length){
	ch = 8;
	write(1, &ch, 1);

	for(int i=cur_loc-1; i<line_length-1; i++){
	  ch = line_buffer[i+1];
	  write(1, &ch ,1);
	  line_buffer[i] = line_buffer[i+1];
	}
	ch = ' ';
	write(1, &ch, 1);
	for(int i=cur_loc-1; i<line_length; i++){
	  ch = 8;
	  write(1, &ch, 1);
	}
        line_length--;
	cur_loc--;
      }

      else{
        // Go back one character
        ch = 8;
        write(1,&ch,1);

        // Write a space to erase the last character read
        ch = ' ';
        write(1,&ch,1);

        // Go back one character
        ch = 8;
        write(1,&ch,1);

        // Remove one character from buffer
        line_length--;
        cur_loc--;
      }
    }
    else if (ch==9) {
      //Tab
      char cwd[128];
      if(getcwd(cwd, sizeof(cwd))!=NULL){
	//strcpy(line_buffer, cwd);
	DIR * d = opendir(cwd);
	struct dirent * ent;
	while((ent = readdir(d))!=NULL){
	  printf(ent->d_name);
	  break;
	  strcpy(line_buffer, ent->d_name);
	}
	//ch = 'c';
	//write(1, &ch, 1);
      }
    }
    else if (ch==27) {
      // Escape sequence. Read two chars more
      //
      // HINT: Use the program "keyboard-example" to
      // see the ascii code for the different chars typed.
      //
      char ch1;
      char ch2;
      read(0, &ch1, 1);
      read(0, &ch2, 1);
      if (ch1==91 && ch2==66) {
	// Down arrow
	int i = 0;
	for(i=0; i < line_length; i++){
	  ch=8;
	  write(1, &ch, 1);
	}
	for(i=0; i < line_length; i++){
	  ch = ' ';
	  write(1, &ch, 1);
	}
	for(i=0; i < line_length; i++){
	  ch = 8;
	  write(1, &ch, 1);
	}
	if(history_index == history_length-1){
	  strcpy(line_buffer, "");
	  line_length = 0;
	}
	else if(history_index < history_length-1){
	  history_index++;
	  strcpy(line_buffer, history[history_index]);
	  line_length = strlen(line_buffer);
	}
	cur_loc = line_length;
	write(1, line_buffer, line_length);
      }
      if (ch1==91 && ch2==65) {
	// Up arrow. Print next line in history.

	// Erase old line
	// Print backspaces
	int i = 0;
	for (i =0; i < line_length; i++) {
	  ch = 8;
	  write(1,&ch,1);
	}

	// Print spaces on top
	for (i =0; i < line_length; i++) {
	  ch = ' ';
	  write(1,&ch,1);
	}

	// Print backspaces
	for (i =0; i < line_length; i++) {
	  ch = 8;
	  write(1,&ch,1);
	}

	// Copy line from history
	strcpy(line_buffer, history[history_index]);
	line_length = strlen(line_buffer);
	history_index--;
	history_index=(history_index)%history_length;
	if(history_index == -1)history_index = history_length-1;
	//history_index=(history_index+1)%history_length;
	cur_loc = line_length;
	// echo line
	write(1, line_buffer, line_length);
      }
      else if (ch1==91 && ch2==67 ) {
	//Left arrow
	if(cur_loc < line_length){
	  ch = 27;
	  write(1,&ch,1);
	  ch = 91;
	  write(1,&ch,1);
	  ch = 67;
	  write(1,&ch,1);
	  cur_loc++;
	}
      }
      else if(ch1==91 && ch2==68 &&cur_loc > 0 ) {
	//Right arrow
	if(cur_loc > 0){
	  ch = 27;
	  write(1,&ch,1);
	  ch = 91;
	  write(1,&ch,1);
	  ch = 68;
	  write(1,&ch,1);
	  cur_loc--;
	}
      }
    }

  }


  // Add eol and null char at the end of string
  line_buffer[line_length]=10;
  line_length++;
  line_buffer[line_length]=0;

  history[history_length] = (char*)malloc(strlen(line_buffer)*sizeof(char)+1);
  strcpy(history[history_length], line_buffer);
  history[history_length][strlen(line_buffer)-1] = '\0';
  history_index = history_length;
  history_length++;


  return line_buffer;
}

