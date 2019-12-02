#!/usr/bin/env python3

import socket

HOST = "127.0.0.1"  # Standard loopback interface address (localhost)
PORT = 65432        # Port to listen on (non-privileged ports are > 1023)

SEND_DATA_TO_CLIENT = False

def send_data_to_client(conn):
    conn.sendall("Server Ok!".encode())


def parse(conn):
    while True:
        data = conn.recv(1024)
        if not data:
            break
        print("Message received: ", data.decode())
        if SEND_DATA_TO_CLIENT:
            send_data_to_client(conn)



with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind((HOST, PORT))
    while True:
        print(f"Waiting for connection in {HOST}:{PORT}")
        s.listen()
        conn, addr = s.accept()
        with conn:
            print("Connected by", addr)
            parse(conn)
            print("Connection Closed!")
