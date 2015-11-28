# Simple Socks5

Simple Socks5 is a proxy based on socks5 protocol. It's currently support CONNECT command of socks5 (TCP CONNECT) and encrypt traffic with 'xor crypt'. 'xor crypt' is not secure, but it can avoid traffic detection.

This project build for me to study network programming, so it's very simple and crude now. I will continued maintain  it until it become perfect and stable.

# Build

Just run:
```
make all
```

# Usage

After build you would get two executable file of `local` and `remote`.

The "local" is a socks5 server. "local" accept connection with socks5 protocol. After create connection, the "local" relay traffic to "remote" with xor crypt, and the "remote" is responsible for connect to destination host.

Run `./remote <server_port>` to start a "remote", it accept connection from `local`.

run  `./local <local_port> <remote_address> <remote_port>` to start a socks5 server. The `<local_port>` is the port of the socks5 server listening. The `<remote_address>` and `<remote_port>` correspond the ip and port of "remote" listening.
