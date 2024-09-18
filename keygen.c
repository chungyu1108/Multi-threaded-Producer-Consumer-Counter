#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define CHAR_SET "ABCDEFGHIJKLMNOPQRSTUVWXYZ "

char randomCharacter() {
    return CHAR_SET[rand() % (sizeof(CHAR_SET) - 1)];
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "USAGE: %s keylength\n", argv[0]);
        return 1;
    }

    int keyLength = atoi(argv[1]);
    if (keyLength <= 0) {
        fprintf(stderr, "Key length must be a positive integer\n");
        return 1;
    }

    srand(time(NULL));
    for (int i = 0; i < keyLength; i++) {
        printf("%c", randomCharacter());
    }
    printf("\n");

    return 0;
}
