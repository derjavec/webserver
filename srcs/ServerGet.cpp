#include "ServerGet.hpp"

std::map<int, ClientState> clientStates;

void ServerGet::handleClientWrite(Server &server, int clientFd)
{
    if (clientStates.find(clientFd) == clientStates.end())
        return;
    ClientState &state = clientStates[clientFd];
    const size_t bufferSize = 8192;
    char buffer[bufferSize];
    while (true)
    {
        ssize_t bytesRead = pread(state.fileFd, buffer, bufferSize, state.offset);
        if (bytesRead == 0)
        {
            std::cout << "Finished sending file for client " << clientFd << std::endl;
            struct epoll_event ev;
            ev.events = EPOLLIN;
            ev.data.fd = clientFd;
            epoll_ctl(server._epollFd, EPOLL_CTL_MOD, clientFd, &ev);
            close(state.fileFd);
            clientStates.erase(clientFd);
            return;
        }
        ssize_t bytesSent = send(clientFd, buffer, bytesRead, 0);
        if (bytesSent == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                std::cout << "Send would block, will try again later." << std::endl;
                struct epoll_event ev;
                ev.events = EPOLLOUT | EPOLLIN;
                ev.data.fd = clientFd;
                epoll_ctl(server._epollFd, EPOLL_CTL_MOD, clientFd, &ev);
                return;
            }
            std::cerr << "Error sending file fragment: " << strerror(errno) << std::endl;
            close(state.fileFd);
            clientStates.erase(clientFd);
            return;
        }
        state.offset += bytesSent;
    }
}


void ServerGet::serveLargeFileAsync(Server &server, int clientFd, const std::string& filePath, const std::string &ContentType)
{
    if (filePath.find('%') != std::string::npos) 
    {
        std::cerr << "❌ Error: Invalid URL encoding in request path: " << filePath << std::endl;
        ServerErrors::handleErrors(server, clientFd, 400);
        return;
    }
    int fileFd = open(filePath.c_str(), O_RDONLY);
    if (fileFd == -1)
    {
        std::cerr << "Error opening file: " << filePath << std::endl;
        std::string response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        send(clientFd, response.c_str(), response.size(), 0);
        return;
    }
    struct stat fileStat;
    fstat(fileFd, &fileStat);

    std::string response = "HTTP/1.1 200 OK\r\n"
                       "Content-Type: " + ContentType + "\r\n"
                       "Set-Cookie: session_id=" + server._clientSessions[clientFd].sessionId + "; Path=/; HttpOnly\r\n"
                       "Content-Length: " + ServerUtils::numberToString(fileStat.st_size) + "\r\n\r\n";

    send(clientFd, response.c_str(), response.size(), 0);
    clientStates[clientFd] = ClientState();
    clientStates[clientFd].fileFd = fileFd;
    clientStates[clientFd].offset = 0;
    clientStates[clientFd].totalSize = fileStat.st_size;
    handleClientWrite(server, clientFd);
}


void serveStaticFile(const std::map<int, SessionData> &clientSessions, Server &server, int clientFd, const std::string &filePath, const std::string &contentType)
{
    std::ifstream file(filePath.c_str(), std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Error: Couldn't open the file " << filePath << std::endl;
        ServerErrors::handleErrors(server, clientFd, 404);
        return;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();
    if (content.empty())
    {
        ServerErrors::handleErrors(server, clientFd, 204);
        return ;
    } 
    std::string setCookieHeader;
    if (clientSessions.find(clientFd) != clientSessions.end())
        setCookieHeader = "Set-Cookie: session_id=" + clientSessions.at(clientFd).sessionId + "; Path=/; HttpOnly\r\n";

    std::string response = "HTTP/1.1 200 OK\r\n";
    response += setCookieHeader;
    response += "Content-Type: " + contentType + "\r\n";
    response += "Content-Length: " + ServerUtils::numberToString(content.size()) + "\r\n";
    response += "\r\n";
    response += content;

    if (send(clientFd, response.c_str(), response.size(), 0) == -1) {
        std::cerr << "Error sending file to client (fd: " << clientFd << "): " << strerror(errno) << std::endl;
    }
}

void ServerGet::handleRawPost(Server& server, int clientFd, std::vector<char>& clientBuffer)
{
    std::string request(clientBuffer.begin(), clientBuffer.end());

    size_t bodyStart = request.find("\r\n\r\n");
    if (bodyStart == std::string::npos)
        return;

    std::string postBody = request.substr(bodyStart + 4);  
    if (postBody.empty())
    {
        ServerErrors::handleErrors(server, clientFd, 400);
        return ;
    }       
    std::ofstream file("outfile.txt", std::ios::app);
    if (file.is_open())
    {
        file << postBody << "\n";
        file.close();
    } 
    else
    {
        ServerErrors::handleErrors(server, clientFd, 400);
        return;
    }
    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Set-Cookie: session_id=" + server._clientSessions[clientFd].sessionId + "; Path=/; HttpOnly\r\n"
        "Content-Length: 0\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";
    send(clientFd, response.c_str(), response.size(), 0);
}


void ServerGet::handleGetRequest(Server &server, int clientFd, HttpRequestParser &parser, const std::string &contentType, std::string &filePath)
{
    std::map<std::string, std::string> headers = parser.getHeaders();
    if (headers.find("Content-Length") != headers.end())
    {
        std::cerr << "❌ 400 Bad Request: GET request should not have Content-Length." << std::endl;
        ServerErrors::handleErrors(server, clientFd, 400);
        return;
    }
    if (headers.find("Transfer-Encoding") != headers.end())
    {
        std::cerr << "❌ 400 Bad Request: GET request should not have Transfer-Encoding." << std::endl;
        ServerErrors::handleErrors(server, clientFd, 400);
        return;
    }
    if (headers.find("Expect") != headers.end() && headers["Expect"] == "100-continue")
    {
        std::cerr << "❌ 417 Expectation Failed: GET request should not use Expect: 100-continue." << std::endl;
        ServerErrors::handleErrors(server, clientFd, 417);
        return;
    }
    std::string url = parser.getPath();
    if (ResolvePaths::isLocation(server, url) && !ResolvePaths::isMethodAllowed(server, filePath, "GET", url))
    {
        std::cerr << "❌ Error: GET not allowed for this location." << std::endl;
        ServerErrors::handleErrors(server, clientFd, 405);
        return;
    }
    std::cout << "url " <<url << std::endl;
    if (ServerUtils::isDirectory(filePath) && ServerFolders::handleFoldersRequests(server, clientFd, filePath, url))
        return;
    size_t fileSize = ServerUtils::getFileSize(filePath);
    if (fileSize > server._clientMaxBodySize)
    {
        serveLargeFileAsync(server, clientFd, filePath, contentType);
        return;
    }
    serveStaticFile(server._clientSessions, server, clientFd, filePath, contentType);
}

