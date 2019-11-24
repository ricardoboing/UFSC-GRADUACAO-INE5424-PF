#!/usr/bin/env python3

import socket

HOST = "127.0.0.1"  # The server"s hostname or IP address
PORT = 65432        # The port used by the server
GPGGA_locations = ["$GPGGA,134658.00,0,N,0.785398,E,2,09,1.0,-6377995.6,M,-16.27,M,08,AAAA*60",#"100,100,0",
                   "$GPGGA,134658.00,1.5708,N,0,E,2,09,1.0,-6356752.3,M,-16.27,M,08,AAAA*60",#"0,0,0",
                   "$GPGGA,134658.00,0,N,1.5708,E,2,09,1.0,-6378036.0,M,-16.27,M,08,AAAA*60"#"0,100,0"
                   ]; #"100,0,50" caso 3d
# GPGGA_locations =["$GPGGA,134658.00,-89.81042,N,45,E,2,09,1.0,-6356852.1,M,-16.27,M,08,AAAA*60",
# "$GPGGA,134658.00,-89.81042,N,45,E,2,09,1.0,-6356852.1,M,-16.27,M,08,AAAA*60",
# "$GPGGA,134658.00,-89.81042,N,45,E,2,09,1.0,-6356852.1,M,-16.27,M,08,AAAA*60"];
RECV_DATA_FROM_SERVER = True

def recv_data_from_server(sock):
    data = s.recv(4)
    print("Received from Back-end: ", data.decode())


def serialize(sock):
    while True:
        mes = input("Message: ")
        if mes == "":
            break
        sock.sendall(mes.encode())
        if RECV_DATA_FROM_SERVER:
            recv_data_from_server(sock)


def recv_gps_request(sock):
    data = s.recv(1024);
    data_int = int(data.decode());
    if(data_int >= 0 and data_int < 3):
        sock.sendall(GPGGA_locations[data_int].encode());
        sock.sendall("%".encode());


with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))
    while True:
        recv_gps_request(s)
    #recv_data_from_server(s);
    #serialize(s)
    print("Connection Closed")

print("Client Ended!")
