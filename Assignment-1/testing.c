#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>



int main()
{
    char palindrome[] = "a9"; // input
    bool is_palindrome = false; 
    int length_of_array = strlen(palindrome) - 1;
    char space[] = " ";
    int i = 0;
    int j = length_of_array;
    if(length_of_array > 2 ){

    while (palindrome[i] != '\0')
    {
        
        while(palindrome[i] == *space)
        {
            i++;
        }
        while(palindrome[j] == *space)
        {
            j--;
        }
        //printf("i:%c & i-1:%c\n", palindrome[i], palindrome[length_of_array - i]);
        if(tolower(palindrome[i]) == tolower(palindrome[j]))
        {
            is_palindrome = true;
        }
        else
        {
            is_palindrome = false;
            break;
        }
        i++;
        j--;
    }
printf(is_palindrome ? "true\n" : "false\n");
    return 0;
}
    else{
        printf("Not long enough");
    }
}


