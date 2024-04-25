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

void printStatusBar(int row, int col, int winHeight, int page, int start,
                    int end);

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
  int pages = numLines / (winHeight - 1);
  int *arrayPages = (int *)malloc((pages + 1) * sizeof(int));
  int index = 0;
  int lim = 0;

  // lines per page
  for (int i = 0; i < pages + 1; i++) {
    arrayPages[i] = i * (winHeight - 1);
  }

  // status bar
  printStatusBar(recorrido, col, winHeight, page, arrayPages[index],
                 arrayPages[index + 1]);

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
    if (c == KEY_UP && recorrido > 0) {
      recorrido -= 1;
      row -= 1;
      if (row == -1 && index > 0) {
        // TODO: repeat code
        // preview page
        index--;

        clear();
        for (int i = arrayPages[index]; i < arrayPages[index + 1]; i++) {
          char *l = hazLinea(map, i * 16);

          move(i, 0);
          addstr(l);
        }

        // restart position
        row = winHeight - 2;
        page--;

        refresh();
        printStatusBar(recorrido, col, winHeight, page, arrayPages[index],
                       arrayPages[index + 1]);
        move(row, col);
        c = getch();
      }
    }

    if (c == KEY_DOWN && recorrido < numLines - 1) {
      recorrido += 1;
      row = row + 1;
      if (row == winHeight - 1) {
        // TODO: repeat code
        index++;

        if (index == pages) {
          lim = numLines - arrayPages[index];
          printf("Aqui el limite es igual a: %d\n", lim);
          lim = arrayPages[index] + lim;
        } else {
          lim = arrayPages[index + 1];
        }

        printf("Aqui el limite es igual a: %d\n", lim);

        clear();
        for (int i = arrayPages[index]; i < lim; i++) {
          char *l = hazLinea(map, i * 16);

          move(i, 0);
          addstr(l);
        }

        // restart position
        row = 0;
        page++;
        refresh();
        printStatusBar(recorrido, col, winHeight, page, arrayPages[index], lim);
        move(row, col);
        c = getch();
      }
    }

    printStatusBar(recorrido, col, winHeight, page, arrayPages[index], lim);
    move(row, col);
    c = getch();
  }

  if (munmap(map, fd) == -1) {
    perror("Error al desmapear");
  }
  close(fd);

  return 0;
}

void printStatusBar(int row, int col, int winHeight, int page, int start,
                    int end) {
  move(winHeight - 1, 0);
  clrtoeol(); // Clear line
  printw("Fila: %d, Columna: %d, Página: %d, start: %d, end: %d", row + 1, col,
         page, start, end);
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
