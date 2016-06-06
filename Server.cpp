#include <netLink.h>
#include <iostream>

bool splitLine(const std::string& str, const std::string& prefix, std::string& postfix) {
    if(str.compare(0, prefix.length(), prefix) != 0)
        return false;
    postfix = str.substr(prefix.length(), str.length()-prefix.length()-2);
    return true;
}

std::set<std::string> users = {"42"};
class Client {
    public:
    bool dataMode = false;
    std::string name, from, recipient, data;
    bool finalize() {
        dataMode = false;
        if(from.length() == 0 || users.find(recipient) == users.end() || data.length() == 0)
            return false;
        std::cout << "RECEIVED MAIL (" << name << ")" << std::endl;
        std::cout << "FROM: " << from << std::endl;
        std::cout << "TO: " << recipient << std::endl;
        std::cout << "DATA: " << data << std::endl;
        return true;
    }
};
std::map<std::shared_ptr<netLink::Socket>, std::unique_ptr<Client>> clients;

int main(int argc, char** argv) {
    #ifdef WIN32
    netLink::init();
    #endif

    netLink::SocketManager socketManager;
    std::shared_ptr<netLink::Socket> socket = socketManager.newSocket();
    socket->initAsTcpServer("*", 25);

    socketManager.onConnectRequest = [&](netLink::SocketManager* manager, std::shared_ptr<netLink::Socket> serverSocket, std::shared_ptr<netLink::Socket> clientSocket) {
        std::cout << "Accepted connection from " << clientSocket->hostRemote << ":" << clientSocket->portRemote << std::endl;

        std::iostream stream(clientSocket.get());
        stream << "220 Service ready\n";
        stream.flush();

        std::unique_ptr<Client> client(new Client());
        clients.insert(std::make_pair(clientSocket, std::move(client)));
        return true;
    };

    socketManager.onStatusChange = [&](netLink::SocketManager* manager, std::shared_ptr<netLink::Socket> clientSocket, netLink::Socket::Status prev) {
        if(socket->getStatus() == netLink::Socket::Status::NOT_CONNECTED) {
            clients.erase(clientSocket);
            std::cout << "Lost connection of " << socket->hostRemote << ":" << socket->portRemote << std::endl;
        } else
            std::cout << "Status of " << socket->hostRemote << ":" << socket->portRemote << " changed from " << prev << " to " << socket->getStatus() << std::endl;
    };

    socketManager.onReceiveRaw = [&](netLink::SocketManager* manager, std::shared_ptr<netLink::Socket> clientSocket) {
        Client* client = clients.find(clientSocket)->second.get();
        std::iostream stream(clientSocket.get());
        std::string received(std::istreambuf_iterator<char>(stream), {});
        if(client->dataMode) {
            if(received != ".\r\n")
                client->data += received;
            else if(client->finalize())
                stream << "250 OK\n";
            else
                stream << "503 Bad sequence of commands\n";
        } else if(splitLine(received, "HELO ", client->name)) {
            stream << "250 OK\n";
        } else if(splitLine(received, "MAIL FROM:", client->from)) {
            stream << "250 OK\n";
        } else if(splitLine(received, "RCPT TT:", client->recipient)) {
            if(users.find(client->recipient) != users.end())
                stream << "250 OK\n";
            else
                stream << "551 Unknown recipient\n";
        } else if(received == "DATA\r\n") {
            stream << "354 Start mail input\n";
            client->dataMode = true;
        } else if(received == "QUIT\r\n") {
            stream << "221 Closing channel\n";
            std::cout << "Lost connection of " << clientSocket->hostRemote << ":" << clientSocket->portRemote << std::endl;
            clients.erase(clientSocket);
            socket->disconnect();
        } else
            stream << "500 Command not recognized\n";
        stream.flush();
    };

    while(socket->getStatus() != netLink::Socket::Status::NOT_CONNECTED)
        socketManager.listen();
    return 0;
}
