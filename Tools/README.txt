This directory contains externally maintained build and
testing tools.

google-toolbox-for-mac-1-5-1:
	Apache License 2.0.

	Provides iPhone unit testing.
	Downloaded from http://code.google.com/p/google-toolbox-for-mac/

    Modified as follows:
    - Patch GTM to remove use of private -[UIApplication terminate] API.
    - Patch GTM to not use the deprecated -[NSString stringWithCString:]
    - Remove all sources except for the iPhone unit test harness.
