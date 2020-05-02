Group Members:
	Samuel Autrey
	Paris Estes
	Trevor Hendricks
	Amy Raney

How to run:
    To compile type make. then to run the server type ./svr <port> and to execute the client type ./cli <machine> <port> <other client ip address>

Organization:
  Amy Raney:        Server
	Trevor Hendricks: Server
	Paris Estes:      Client
  Samuel Autrey:    Client

Design Overview:
    Our code is seperated by two files: 
    1) Major2svr.c 
    2) Major2cli.c

    Major2cli.c:
        it initially connects to the server and waits for input from either the terminal or the server
        if it recieves input from the terminal: 
            it sends it to the server
            recieves the total from the server
        if it recieves input from the server:
            we decide if the client will become client-client or client-server
            client-client:
                disconnects from server and connects to client-server
                send current total
                disconnects from client-server
                return 0
            client-server:
                creates a thread to become a psudo server
                connects to client-client
                recieves the client-client total
                exits thread and gives main the client-client total
                stays connected to server
    in Major2svr.c:


	
Complete Specification:
    our server allows for up to two clients to connect and if two are connected then no more will be connected. Also a client can connect and disconnect at will
    a client can enter 0 to disconnect manually

Known Bugs:
