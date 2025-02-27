#include <stdio.h>

int main()
{
    printf("Content-Type: text/html\n\n");

    printf("<!DOCTYPE html>");
    printf("<html lang='en'>");
    printf("<head><meta charset='UTF-8'><title>C CGI Test</title></head>");
    printf("<body><h1>Hello from C CGI Script!</h1></body>");
    printf("</html>");

    return 0;
}