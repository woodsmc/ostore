# ostorecmd Command Line Tool

## Overview

The OStore command line tool allows the creation and inspection of an Ostore file. This tool has been created to allow developers to create an OStore file during build time. 

# Purpose of this Document

This document describes the OStore command line tool and how to use it. It does not define the file format which is detailed in a separate document. **TODO: INSERT LINK**



## Document Version History

| Version | Date            | Author      | Comment              |
| ------- | --------------- | ----------- | -------------------- |
| 0.1     | October 8 2020  | Chris Woods | Initial tool outline |
| 0.2     | October 25 2020 | Chris Woods | Update for usage     |
|         |                 |             |                      |

## Tool Usage

If invoked with no parameters `ostorecmd` will respond with the tool usage information:

```
ostorecmd Command Line Tool, version 1.0 (c) Copyright Chris Woods 2020
oStore library version 1.0 (c) Copyright Chris Woods 2020
   ostorecmd <filename> (-create | -list | -extract | -insert ) <ID> (FILE | TEXT) [<outfile> | <text>]	
```

### Create a file

```
ostorecmd <filename> -create
```

This will create a OStore with the file name `<filename>` .
```
Creating file <filename> ... [ok]
```

Upon an error the tool will report

```
ostorecmd Version 1.0 by Chris Woods
Creating file <filename> ... [error <error code>]
```

Where `<error code>` will be one of the error codes outlined below.



### List the contents of a file

```
ostorecmd <filename> -list
```

This will list the contents of an OStore file.  The output will look as follows:

```
opening test.store... [ok]
number of objects... [ok 2 objects]
------------------------------------
  Index |   ID   |  Type  | Length  
------------------------------------
       0|       0|   index|     128
       1|       1|   trash|       0
------------------------------------
```

Note that all stores have two objects used for house keeping, these are the index (0) and the trash (1) attempts to use these object IDs for user data will result in an error.

### Extract an object from a file

```
ostorecmd <filename> -extract <ID> (FILE | TEXT ) [<outfile>]
```

This will extract the contents of an object either to the console as text when the TEXT type is specified, or to another file with the FILE type is specified. 

`<ID>`  : This specifies the object ID to extract.

`<outfile>`: This specifies the file to create with the contents of the object, this is only needed if `FILE` is specified as the type.

#### Example : Extract text to console

```
ostorecmd my.store -extract 12 TEXT
```

The example above will extract the contents of object 12 to the console as text, and produces output similar too:

```
opening my.store... [ok]
checking for object with id 12 and getting it's length ...length is 128 [ok]
reading object...this is an exampe of some text[ok]
```

#### Example: Extract object to file

```
ostorecmd my.store -extract 14 FILE out.png
```

This example above will extract the contents of object 12 and store them as a file called `out.png` and produces output similar to the following:

```
opening my.store... [ok]
checking for object with id 14 and getting it's length ...length is 1371136 [ok]
reading object...[ok]
```



### Insert or Overwrite an Object

```
ostorecmd <filename> -insert <ID> (FILE | TEXT ) (<outfile> | <text>)
```

This will insert an object with the content specified, either text from the command line or the contents of a file. 

#### Example: Insert text to object

```
ostorecmd my.store -insert 12 TEXT "this is an exampe of some text"
```

This will insert, or overwrite it if already exists an object with the ID of 12 into the file `my.store` with the contents `this is an example of some text`. Note that the quotation marks are not stored. The command line tool will return the following:

```
opening my.store... [ok]
removing existing object with ID 12...  [not found - new entry]
adding object with ID 12, and length 31...[ok]
writing data to object 12.. [ok]
```



#### Example: Insert file as object

```
ostorecmd my.store -insert 14 FILE ./photo.png
```

This will insert, or overwrite if it already exists an object with the ID of 12 into the file `my.store` with the contents of the file `./photo.png`. The produces output similar to this:

```
opening my.store... [ok]
removing existing object with ID 14...  [not found - new entry]
adding object with ID 14, and length 1371092...[ok]
writing data to object 14 .... [ok]
```



### Error Codes

The following are a list of the possible error codes that could be reported:

| Code | Meaning                                          |
| ---- | ------------------------------------------------ |
| 0    | No error, operation completed ok                 |
| -1   | Not found                                        |
| -2   | General error (an unspecified error)             |
| -3   | Overflow / out of disk space etc                 |
| -4   | Memory error, a malloc failed - no available RAM |
| -5   | Corrupt, the file being used is corrupt          |
| -6   | Already exists                                   |