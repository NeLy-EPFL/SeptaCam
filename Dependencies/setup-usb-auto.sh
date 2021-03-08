#!/usr/bin/expect
set timeout 1
spawn "./setup-usb.sh"
expect { "*Continue setup? *" }
send yes\n
expect { send “no\n” }
