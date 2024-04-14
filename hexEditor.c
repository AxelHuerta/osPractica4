#include <ctype.h>
#include <curses.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

/* Variable global para mejor legibilidad */
int fd; // Archivo a leer

void printStatusBar(int row, int col, int winHeight, int page);

char *hazLinea(char *base, int dir) {
  char linea[100]; // La linea es mas pequeña
  int o = 0;
  // Muestra 16 caracteres por cada linea
  o += sprintf(linea, "%08x ", dir); // offset en hexadecimal
  for (int i = 0; i < 4; i++) {
    unsigned char a, b, c, d;
    a = base[dir + 4 * i + 0];
    b = base[dir + 4 * i + 1];
    c = base[dir + 4 * i + 2];
    d = base[dir + 4 * i + 3];
    o += sprintf(&linea[o], "%02x %02x %02x %02x ", a, b, c, d);
  }
  for (int i = 0; i < 16; i++) {
    if (isprint(base[dir + i])) {
      o += sprintf(&linea[o], "%c", base[dir + i]);
    } else {
      o += sprintf(&linea[o], ".");
    }
  }
  sprintf(&linea[o], "\n");

  return (strdup(linea));
}

char *mapFile(char *filePath) {
  /* Abre archivo */
  fd = open(filePath, O_RDONLY);
  if (fd == -1) {
    perror("Error abriendo el archivo");
    return (NULL);
  }

  /* Mapea archivo */
  struct stat st;
  fstat(fd, &st);
  long fs = st.st_size;

  char *map = mmap(0, fs, PROT_READ, MAP_SHARED, fd, 0);
  if (map == MAP_FAILED) {
    close(fd);
    perror("Error mapeando el archivo");
    return (NULL);
  }

  return map;
}

int leeChar() {
  int chars[5];
  int ch, i = 0;
  nodelay(stdscr, TRUE);
  while ((ch = getch()) == ERR)
    ; /* Espera activa */
  ungetch(ch);
  while ((ch = getch()) != ERR) {
    chars[i++] = ch;
  }
  /* convierte a numero con todo lo leido */
  int res = 0;
  for (int j = 0; j < i; j++) {
    res <<= 8;
    res |= chars[j];
  }
  return res;
}

int edita(char *filename) {

  /* Limpia pantalla */
  clear();

  /* Lee archivo */
  char *map = mapFile(filename);
  if (map == NULL) {
    exit(EXIT_FAILURE);
  }

  // TODO: check this and last empty text
  // Obtén el tamaño del archivo
  struct stat st;
  fstat(fd, &st);
  long fileSize = st.st_size;
  int numLines = fileSize / 16;

  // TODO: unnecessary height
  // Obtiene el tamaño de la ventana
  int winHeight, winWidth;
  getmaxyx(stdscr, winHeight, winWidth);

  for (int i = 0; i < numLines && winHeight - 1; i++) {
    // Haz linea, base y offset
    char *l = hazLinea(map, i * 16);
    move(i, 0);
    addstr(l);
  }
  refresh();

  int col = 9;
  int row = 0;
  int scroll = winHeight - 1;
  int recorrido = 0;
  int page = 1;

  // status bar
  printStatusBar(row, col, winHeight, page);

  // NOTE: move before getch
  move(row, col);

  int c = getch();

  while (c != 26) {
    if (c == KEY_LEFT && col > 9) {
      col = col - 3;
    }
    if (c == KEY_RIGHT && col < 54) {
      col = col + 3;
    }
    // TODO: add recorrido to the condition
    if (c == KEY_UP) {
      if (row == 0 && scroll > 0) {
        // TODO: repeat code
        // next page
        scroll -= winHeight - 1;
        clear();
        for (int i = scroll; i < (scroll + winHeight - 1); i++) {
          char *l = hazLinea(map, i * 16);

          move(i + scroll, 0);
          addstr(l);
        }

        // next page
        // scroll -= winHeight - 1;
        refresh();

        // restart position
        col = 9;
        row = winHeight - 2;
        page = scroll / (winHeight - 1);
        printStatusBar(row, col, winHeight, page);
        move(row, col);
        c = getch();
      } else {
        row = row - 1;
      }

      recorrido -= 1;
    }
    if (c == KEY_DOWN && recorrido < numLines - 1) {
      recorrido += 1;
      row = row + 1;
      if (row == winHeight - 1) {
        // TODO: repeat code
        clear();
        for (int i = scroll; i < (scroll + winHeight - 1) && i < numLines;
             i++) {
          char *l = hazLinea(map, i * 16);

          move(i + scroll, 0);
          addstr(l);
        }

        // next page
        scroll += winHeight - 1;

        // restart position
        col = 9;
        row = 0;
        page = scroll / (winHeight - 1);
        refresh();
        printStatusBar(row, col, winHeight, page);
        move(row, col);
        c = getch();
      }
    }

    printStatusBar(row, col, winHeight, page);
    move(row, col);
    c = getch();
  }

  if (munmap(map, fd) == -1) {
    perror("Error al desmapear");
  }
  close(fd);

  return 0;
}

void printStatusBar(int row, int col, int winHeight, int page) {
  move(winHeight - 1, 0);
  clrtoeol(); // Clear line
  printw("Fila: %d, Columna: %d, Página: %d", row + 1, col, page);
  refresh();
}

int main(int argc, char const *argv[]) {
  initscr();
  raw();
  keypad(stdscr, TRUE); /* Para obtener F1,F2,.. */
  noecho();

  /* El archivo se da como parametro */
  if (argc != 2) {
    printf("Se usa %s <archivo> \n", argv[0]);
    return (-1);
  }

  edita((char *)argv[1]);

  endwin();

  return 0;
}