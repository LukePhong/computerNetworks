#include "client.h"
#include "server.h"

//extern "C" int main_client(int, char **);
//
//extern "C" int main_server(int, char **);

int main(int c, char **v) {
    if (c == 3) {
        main_client(c, v);
    } else {
        main_server(c, v);
    }
    return 0;

}
