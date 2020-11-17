# OStore Code Error Handling

Handling "c" code which calls functions in which each function returns a status code, or success code can be a pain. Ideally we'd check every single return / status code from every function we invoke. That's possible, but it tends to produce code which is a see of nested if statements.

In an effort to avoid this, the oStore code base uses a set of macros as follows:

```c
#define START int retval = 0
#define VALIDATE( A, B) if ( (A) ) HANDLE_ERROR(B)
#define IF_NOT_OK_HANDLE_ERROR( A ) if ( (retval = (A)) != ERR_OK ) HANDLE_ERROR( retval )
#define HANDLE_ERROR(A) { retval = A; goto HandleError; };
#define PROCESS_ERROR return retval; HandleError:
#define FINISH return retval;
```

These may seem weird at first, but they do allow us to write code which can handle errors a bit more gracefully:

```c
int exampleFunction(char** returnObj) {
    char* buffer = NULL;
    START;
    buffer = malloc(100);
    VALIDATE(buffer == NULL, NO_MEM);
    retval = callSomeFunction(buffer);
    IF_NOT_OK_HANDLE_ERROR(retval);
    (*returnObj) = buffer;
    
    PROCESS_ERROR;
    // something bad happened, clean up here
    if ( buffer ) {
        free(buffer);
    }
    (*returnObj) = NULL;
    FINISH;
}
```

If we expand the macro's you can see exactly what is going on.... 

```c
int exampleFunction(char** returnObj) {
    char* buffer = NULL;
    // START;
    int retval = 0; // 0 is ERR_OK 
    
    buffer = malloc(100);
    
    // VALIDATE(buffer == NULL, NO_MEM);
    if ( buffer == NULL ) { retval = NO_MEM; goto HandleError; }
    
    retval = callSomeFunction(buffer);
    //IF_NOT_OK_HANDLE_ERROR(retval);
    if ( retval != ERR_OK ) { goto HandleError; }
    
    // If we get this far, everything is ok, so return the buffer
    (*returnObj) = buffer;
    // the function will automatically return retval (ERR_OK).
    
    //PROCESS_ERROR;
    return retval;
    HandleError: // label <--
    // something bad happened, clean up here
    if ( buffer ) {
        free(buffer);
    }
    (*returnObj) = NULL;
    
    //FINISH;
    return retval;
}
```



As shown in the `ostore_create` function, this allows us to write code which can verify the return code of a function, and on an error jump to an error handling routine, before gracefully exiting the function call.

