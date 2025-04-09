The point of this repository is to demonstrate that reading/writing from a pseudo-terminal (opened using posix_openpt()) handles the backspace control character differently under z/OS than it does under Linux (and other POSIX operating systems, apparently).

The scenario demonstrated in this repository is as follows:
1) posix_openpt() is used to open the master/pty side of a pseudo terminal.  This is followed by the use of grantpt() and unlockpt() to "grant access to the slave pseudo-terminal device" and "unlock a pseudo-terminal master/slave pair", as described on various web sites.
2) ptsname() is used to retrieve the name of the slave pseudoterminal, which is then passed to open() in order to retrieve a file descriptor value for the slave.
3) On z/OS (MVS), additional steps are performed to make sure, when running in ASCII mode, the appropriate conversions between ASCII (CCSID 819) and EBCDIC (CCID 1047) are performed.  See functions zos_convert_query() and zos_convert_set() in the program source code.
4) With the setup processes above complete, we continue with the main processes.
5) We write to the pty/master fd a string that contains a backspace character.  Specifically, the C string "0129\b3\n".  From a logical perspective, when written to an actual terminal, it would show "0129" for a split second, then the backspace would move the cursor back to the position where the character '9' is present, and would then overwrite it with the character '3'.  This the final result shown on the terminal would simply be "0123", followed by a new line.
6) We then read the data from the pty/master fd.
7) We then read the data from the tty/slave fd.

These last two steps are where the results differ between most POSIX environments and the z/OS POSIX environment.
For step 6, the read result on Linux is exactly what is written.  That is, characters 0, 1, 2, 9, backspace, 3, and linefeed
For step 6, the read result on z/OS is, instead, characters 0, 1, 2, 3, linefeed.  This represents the same "logical" result, but not the same "actual" result as with Linux.

For step 7, the read result on Linux are the characters 0, 1, 2, caret symbol (^), capital H, 3, carriage return, linefeed.  The "^H" is the "text representation" of the backspace character (ASCII code 8).  But it is the actual caret followed by captial-H, not the actual ASCII code 8.
For step 7, the read result on z/OS are the characters 0, 1, 2, 9, backspace, space, backspace, 3, linefeed.  The "^H" values are not returned in this case.

So the questions in the end are:
1) Is z/OS working as designed (and why/why not)?
2) Are there any attributes that can be set on z/OS for it to get the same results as on other POSIX systems?

It's worth noting that the different behavior does not seem to cause "bad" results on z/OS when the pseudo-terminal is used in a real TUI (terminal user interface) program.  At least not that I have observed.

I should also note that the "missing" carriage return character (ASCII code x'0D') in the z/OS results for step 7 has a fix.  I simply did not implement it here.  That fix is to , when executing on z/OS, use Termios processes to set the ONLCR (Map NL to CR-NL) and OPOST (Post process output) options.  When these are set, when an EBCDIC "NL" is read from the slave/tty it is converted to an CR-NL sequence, which is converted to CR-LF when the EBCDIC to ASCII conversion is done.  Or something like that.

Note: When compiling on z/OS, use ASCII mode.  For clang this would be the following:
clang -fzos-le-char-mode=ascii
Other z/OS compilers may use different options to accomplish this.
