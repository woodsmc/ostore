# OStore Command Line Tool

## Overview

The OStore command line tool allows the creation and inspection of an Ostore file. This tool has been created to allow developers to create an OStore file during build time. 

# Purpose of this Document

This document describes the OStore command line tool and how to use it. It does not define the file format which is detailed in a separate document. **TODO: INSERT LINK**



## Document Version History

| Version | Date           | Author      | Comment              |
| ------- | -------------- | ----------- | -------------------- |
| 0.1     | October 8 2020 | Chris Woods | Initial tool outline |
|         |                |             |                      |
|         |                |             |                      |

## Tool Usage

```
ostore <filename> (-create | -list | -extract | -insert ) <ID> (FILE | TEXT) [<outfile> | <text>]	

```

### Create a file

```
ostore <filename> -create
```

This will create a OStore with the file name `<filename>` .
```
OStore Version 1.0 by Chris Woods
Creating file <filename> ... OK
```

Upon an error the tool will report

```
OStore Version 1.0 by Chris Woods
Creating file <filename> ... Error <reason>
```

Where `<reason>` will be one of:

- Destination unreachable

- No permission

  

### List the contents of a file

```
ostore <filename> -list
```

This will list the contents of an OStore file. 

### Extract an object from a file

```
ostore <filename> -extract <ID> (FILE | TEXT ) [<outfile>]
```

This will extract the contents of an object either to the console as text when the TEXT type is specified, or to another file with the FILE type is specified. 

`<ID>`  : This specifies the object ID to extract.

`<outfile>`: This specifies the file to create with the contents of the object, this is only needed if `FILE` is specified as the type.

#### Example : Extract text to console

```
ostore my.store -extract 12 TEXT
```

The example above will extract the contents of object 12 to the console as text.

#### Example: Extract object to file

```
ostore my.store -extract 12 FILE out.png
```

This example above will extract the contents of object 12 and store them as a file called `out.png`.

### Insert or Overwrite an Object

```
ostore <filename> -insert <ID> (FILE | TEXT ) (<outfile> | <text>)
```

This will insert an object with the content specified, either text from the command line or the contents of a file. 

#### Example: Insert text to object

```
ostore my.store -insert 12 TEXT "this is an exampe of some text"
```

This will insert, or overwrite it if already exists an object with the ID of 12 into the file `my.store` with the contents `this is an example of some text`. Note that the quotation marks are not stored.

#### Example: Insert file as object

```
ostore my.store -insert 12 FILE ./photo.png
```

This will insert, or overwrite if it already exists an object with the ID of 12 into the file `my.store` with the contents of the file `./photo.png`

### Version Information

This is available by invoking the tool with no arguments.

```
ostore
```

This will produce the following message:

```
oStore Version 1.0
Written by Chris Woods, October 2020.
```