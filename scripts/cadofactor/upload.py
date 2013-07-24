#!/usr/bin/env python3

# CGI script to handle file uploads

debug=1

import cgi, os
import sys
from tempfile import mkstemp
import sqlite3
import wudb

def diag(level, text, var = None):
    if debug > level:
        if var == None:
            print (text, file=sys.stderr)
        else:
            print (text + str(var), file=sys.stderr)
        sys.stderr.flush()

def analyze(level, name, o):
    """ Dump tons of internal data about an object """
    if debug > level:
        diag (level, "*** Content dump of " + name)
        diag (level, "type(" + name + "): ", type(o))
        diag (level, "dir(" + name + "): ", dir(o))
        diag (level, name + " = ", o);
        diag (level, name + ".__repr__() = ", o.__repr__());
        for n in dir(o):
            diag (level, name + "." + n + " = ", getattr(o, n))

# Global variable in this module so that other Python modules can import
# it and store the path to the upload directory in the shell environment 
# variable specified here
UPLOADDIRKEY="UPLOADDIR"
DBFILENAMEKEY="DBFILENAME"

def do_upload(dbfilename, input = sys.stdin, output = sys.stdout):
    diag(1, "Command line arguments:", sys.argv)
    diag(2, "Environment:", os.environ)

    try: # Windows needs stdio set for binary mode.
        import msvcrt
        msvcrt.setmode (0, os.O_BINARY) # stdin  = 0
        msvcrt.setmode (1, os.O_BINARY) # stdout = 1
    except ImportError:
        pass

    diag(1, "Reading POST data\n", "")

    form = cgi.FieldStorage(fp = input)
    analyze(2, "form", form)

    charset = "utf-8"
    header = "Content-Type: text/plain; charset=" + charset + "\r\n\r\n"

    # A nested FieldStorage instance holds the file
    message = None
    if "WUid" not in form:
        message = 'No "WUid" key found in POST data'
    if "clientid" not in form:
        message = 'No "clientid" key found in POST data'
    elif UPLOADDIRKEY not in os.environ:
        message = 'Script error: Environment variable ' + UPLOADDIRKEY + ' not set'
    elif not os.path.isdir(os.environ[UPLOADDIRKEY]):
        message = 'Script error: ' + os.environ[UPLOADDIRKEY] + ' is not a directory'
    else:
        WUid = form['WUid']
        clientid = form['clientid']
        if 'errorcode' in form:
            errorcode = form['errorcode'].value
            diag(1, "errorcode = ", errorcode)
        else:
            errorcode = None
        if 'failedcommand' in form:
            failedcommand = form['failedcommand'].value
            diag(1, "failedcommand = ", failedcommand)
        else:
            failedcommand = None

        # Test if WUid and clientid was set:
        if not WUid.value:
            message = 'No workunit was specified'
        elif not clientid.value:
            message = 'No client id was specified'

        diag(1, "WUid = ", WUid.value)
        diag(1, "clientid = ", clientid.value)

    if not message:
        filetuples = []
        if 'results' in form:
            fileitem = form['results']
            if isinstance(fileitem, cgi.FieldStorage):
                fileitem = [fileitem] # Make it iterable
        else:
            fileitem = []
            diag(1, 'No "results" form found')

        analyze (2, "fileitem", fileitem)

        message = ""
        for f in fileitem:
            analyze (2, "f", f)

            diag(1, "Processing file ", f.filename)
            # strip leading path from file name to avoid directory traversal attacks
            basename = os.path.basename(f.filename)
            # Make a file name which does not exist yet and create the file
            (fd, filename) = mkstemp(prefix=basename + '.', 
                dir=os.environ[UPLOADDIRKEY])
            filestuple = (f.filename, filename)
            if False:
                filestuple = (f.filename, os.basename(filename))
            filetuples.append(filestuple)
            
            # fd is a file descriptor, make a file object from it
            file = os.fdopen(fd, "wb")
            file.write(f.file.read())
            bytes = file.tell()
            file.close()
            
            # Example output:
            # upload.py: The file "testrun.polyselect.0-5000" for workunit 
            # testrun_polyselect_0-5000 was uploaded successfully by client 
            # localhost and stored as /localdisk/kruppaal/work/testrun.upload/
            # testrun.polyselect.0-5000.kcudj7, received 84720 bytes.
            message = message + 'The file "' + basename + '" for workunit ' + WUid.value + \
                ' was uploaded successfully by client ' + clientid.value + \
                ' and stored as ' + filename + ', received ' + str(bytes) + ' bytes.\n'
        wu = wudb.WuAccess(dbfilename)
        try:
            wu.result(WUid.value, clientid.value, filetuples, errorcode, 
                      failedcommand)
        except wudb.StatusUpdateError:
            message = 'Workunit ' + WUid.value + 'was not currently assigned'
        else:
            message = message + 'Workunit ' + WUid.value + ' completed.\n'

    diag (0, sys.argv[0] + ': ', message.rstrip("\n"))
    if output == sys.stdout:
        output.write(header + message)
    else:
        output.write((header + message).encode(charset))


# If this file is run directly by Python, call do_upload()
if __name__ == '__main__':
    if debug > 0:
      import cgitb; cgitb.enable()

    if DBFILENAMEKEY not in os.environ:
        message = 'Script error: Environment variable ' + DBFILENAMEKEY + ' not set'
    dbfilename = os.environ[DBFILENAMEKEY]
    do_upload(dbfilename)
