# Root Makefile

.PHONY: all Client Server clean

all: Client Server

Client:
	$(MAKE) -C Client

Server:
	$(MAKE) -C Server

clean:
	$(MAKE) -C Client clean
	$(MAKE) -C Server clean