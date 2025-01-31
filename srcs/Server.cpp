#include "Server.hpp"

Server::Server(const ServerConfig& config): _serverFd(-1), _port(config.getPort()), _running(false) , _root(config.getRoot())
{
    _serverName = config.getServerName();
    _clientMaxBodySize = config.getClientMaxBodySize();
    _index = config.getIndex();
    _autoindex = config.isAutoindexEnabled();
    _errorPages = config.getErrorPages();
    _locations = config.getLocations();
    memset(&_address, 0, sizeof(_address));
    _address.sin_family = AF_INET;
    _address.sin_port = htons(_port);
    _address.sin_addr.s_addr = INADDR_ANY;
    try
    {
        initSocket();
        bindSocket();
        startListening();
        initEpoll();
    }
    catch (...)
    {
        stop();
        throw;
    }

   // print();
}
Server::~Server()
{
    stop();
}
Server::Server(const Server& obj)
{
    *this = obj;
}
Server& Server::operator=(const Server& obj)
{
    if (this != &obj)
    {
        _serverFd = obj._serverFd;
        _address = obj._address;
        _ip = obj._ip;
        _port = obj._port;
        _epollFd = obj._epollFd;
        _running = obj._running;
        _serverName = obj._serverName;
        _root = obj._root;
        _clientMaxBodySize = obj._clientMaxBodySize;
        _index = obj._index;
        _autoindex = obj._autoindex;
        _errorPages = obj._errorPages;
        _locations = obj._locations;
    }
    return *this;
}

void Server::initSocket()
{
    _serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverFd == -1)
        throw ServerException("Failed to create socket: " + std::string(strerror(errno)));
    int flags = fcntl(_serverFd, F_GETFL, 0);
    if (flags == -1)
        throw ServerException("Failed to get socket flags: " + std::string(strerror(errno)));
    if (fcntl(_serverFd, F_SETFL, flags | O_NONBLOCK) == -1)
        throw ServerException("Failed to set socket non-blocking: " + std::string(strerror(errno)));
    std::cout << "Socket created successfully." << std::endl;
}

void Server::bindSocket()
{
    if (bind(_serverFd, (struct sockaddr *)&_address, sizeof(_address)) < 0)
        throw ServerException("Failed to bind socket: " + std::string(strerror(errno)));
    std::cout << "Socket bound to port " << _port << std::endl;
}

void Server::startListening()
{
    if (listen(_serverFd, SOMAXCONN) < 0)
        throw ServerException("Failed to start listening: " + std::string(strerror(errno)));
    _running = true;
    std::cout << "Server is listening on port " << _port << std::endl;
}

void Server::initEpoll()
{
     _epollFd = epoll_create1(0);
     if (_epollFd == -1)
        throw ServerException("Failed to create epoll instance: " + std::string(strerror(errno)));
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = _serverFd;
    if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, _serverFd, &ev) == -1)
        throw ServerException("Failed to add server socket to epoll: " + std::string(strerror(errno)));
    std::cout << "Epoll instance created and server socket registered." << std::endl;
}

int Server::acceptClient(sockaddr_in &clientAddress)
{
    socklen_t clientLen = sizeof(clientAddress);
    int clientFd = accept(_serverFd, (struct sockaddr *)&clientAddress, &clientLen);
    if (clientFd == -1)
        throw ServerException("Failed to accept client: " + std::string(strerror(errno)));
    else
    {
        int flags = fcntl(clientFd, F_GETFL, 0);
        if (flags == -1)
        {
            close(clientFd);
            throw ServerException("Failed to get client socket flags: " + std::string(strerror(errno)));
        }
        if (fcntl(clientFd, F_SETFL, flags | O_NONBLOCK) == -1)
        {
            close(clientFd);
            throw ServerException("Failed to set client socket non-blocking: " + std::string(strerror(errno)));
        }
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = clientFd;
        if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, clientFd, &ev) == -1)
        {
            close(clientFd);
            throw ServerException("Failed to add client to epoll: " + std::string(strerror(errno)));
        }
        std::cout << "New client connected (fd: " << clientFd << ")" << std::endl;
    }
    return (clientFd);
}

std::string Server::getStatusMessage(int code)
{
    switch (code)
    {
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 500: return "Internal Server Error";
        default: return "Error";
    }
}

void Server::handleErrors(int clientFd, int code)
{
    if (_errorPages.find(code) == _errorPages.end())
    {
        std::cerr << "Error code not found in configuration: " << code << std::endl;
        std::string response = "HTTP/1.1 " + numberToString(code) + " Error\r\nContent-Length: 0\r\n\r\n";
        if (send(clientFd, response.c_str(), response.size(), 0) == -1)
            std::cerr << "Error sending generic " << code <<" response to client (fd: " << clientFd << "): " << strerror(errno) << std::endl;
        return;
    }
    std::string errorFilePath = _root + _errorPages[code];
    std::ifstream errorFile(errorFilePath.c_str());
    if (errorFile.is_open())
    {
        std::stringstream errorBuffer;
        errorBuffer << errorFile.rdbuf();
        std::string errorContent = errorBuffer.str();
        errorFile.close();
        std::string response = "HTTP/1.1 " + numberToString(code) + " " + getStatusMessage(code) + "\r\n";
        response += "Content-Type: text/html\r\n";
        response += "Content-Length: " + numberToString(errorContent.size()) + "\r\n";
        response += "\r\n";
        response += errorContent;
        if (send(clientFd, response.c_str(), response.size(), 0) == -1)
            std::cerr << "Error sending " << code << " error page to client (fd: " << clientFd << "): " << strerror(errno) << std::endl;

    }
    else
    {
        std::cerr << "Error page not found: " << errorFilePath << std::endl;
        std::string response = "HTTP/1.1 " + numberToString(code) + " " + getStatusMessage(code) + "\r\n";
        std::string genericContent = "<html><body><h1>" + numberToString(code) + " " + getStatusMessage(code) + "</h1></body></html>";
        response += "Content-Type: text/html\r\n";
        response += "Content-Length: " + numberToString(genericContent.size()) + "\r\n\r\n";
        response += genericContent;
        if (send(clientFd, response.c_str(), response.size(), 0) == -1)
            std::cerr << "Error sending generic " << code <<" response to client (fd: " << clientFd << "): " << strerror(errno) << std::endl;
    }
}


std::string Server::generateAutoindexPage(const std::string& directoryPath)
{
    std::string absolutePath = _root + directoryPath;
    DIR* dir = opendir(absolutePath.c_str());
    if (!dir)
        return "<html><body><h1>500 Internal Server Error</h1></body></html>";
    std::string page = "<html><body><h1>Index of " + absolutePath + "</h1><ul>";
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL)
    {
        std::string fileName = entry->d_name;
        page += "<li><a href=\"" + fileName + "\">" + fileName + "</a></li>";
    }
    closedir(dir);
    page += "</ul></body></html>";
    return page;
}

void Server::handleAutoIndex(int clientFd, bool autoindexEnabled, std::string path)
{
    if (autoindexEnabled)
    {
        std::string directoryListing = generateAutoindexPage(path);
        std::string response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: text/html\r\n";
        response += "Content-Length: " + numberToString(directoryListing.size()) + "\r\n";
        response += "\r\n";
        response += directoryListing;
        std::cout << "respuesta enviada: "<< response << std::endl;
        send(clientFd, response.c_str(), response.size(), 0);
    }
    else
        handleErrors(clientFd, 403);
    return ;
}

void Server::handleFoldersRequests(int clientFd, const std::string &path, const std::string &fileType)
{
    std::string locationIndex;
    bool autoindexEnabled = false;
    bool found = false;
    std::string filePath;
    for (std::vector<LocationConfig>::const_iterator it = _locations.begin(); it != _locations.end(); ++it)
    {
        if (normalizePath(path) == normalizePath(it->getPath()))
        {
            locationIndex = it->getIndex();
            filePath = _root + path + locationIndex;
            autoindexEnabled = it->isAutoindexEnabled();
            found = true;
            break;
        }

    }
    if (!found)
    {
         handleErrors(clientFd, 404);
         return ;
    }
    if (locationIndex.empty())
    {
        handleAutoIndex(clientFd, autoindexEnabled, path);
        return ;
    }
    else
    {
        filePath = _root + path + locationIndex;
        std::ifstream file(filePath.c_str());
        if (file.is_open())
        {
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string content = buffer.str();
            file.close();
            std::string response = "HTTP/1.1 200 OK\r\n";
            response += "Content-Type: "+ fileType + "\r\n";
            response += "Content-Length: " + numberToString(content.size()) + "\r\n";
            response += "\r\n";
            response += content;
            if (send(clientFd, response.c_str(), response.size(), 0) == -1)
                std::cerr << "Error sending message to client (fd: " << clientFd << "): " << strerror(errno) << std::endl;
        }

    }
}

void Server::handleFileUpload(int clientFd, const std::string& request)
{
    std::string boundary;
    std::string filename;
    std::string fileContent;
    
    size_t boundaryPos = request.find("boundary=");
    if (boundaryPos == std::string::npos)
    {
        std::cerr << "No boundary found in request" << std::endl;
        return;
    }
    boundary = "--" + request.substr(boundaryPos + 9);

    size_t filenamePos = request.find("filename=\"");
    if (filenamePos != std::string::npos)
    {
        filenamePos += 10;
        size_t endPos = request.find("\"", filenamePos);
        filename = request.substr(filenamePos, endPos - filenamePos);
    } 
    else
    {
        std::cerr << "No filename found" << std::endl;
        return;
    }

    size_t fileStart = request.find("\r\n\r\n", filenamePos) + 4;
    size_t fileEnd = request.find(boundary, fileStart) - 2;
    fileContent = request.substr(fileStart, fileEnd - fileStart);
    for (std::vector<LocationConfig>::const_iterator it = _locations.begin(); it != _locations.end(); ++it)
    {
        if (normalizePath(it->getPath()) == "/upload")
        {
            std::string filePath = "www/upload/" + filename;
            std::ofstream outFile(filePath.c_str(), std::ios::binary);
            if (!outFile)
            {
                std::cerr << "Error opening file for writing" << std::endl;
                return;
            }
            outFile.write(fileContent.c_str(), fileContent.size());
            outFile.close();

            std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
            response += "<h1>Upload Successful</h1>";
            response += "<p>File uploaded: " + filename + "</p>";
            send(clientFd, response.c_str(), response.size(), 0);
        }
        else
            handleErrors(clientFd, 404);

    }
    
}

void Server::executeCGI(int clientFd, const std::string &scriptPath, const std::string &method, const std::string &body)
{
    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        std::cerr << "Error creating pipe" << std::endl;
        return;
    }
    pid_t pid = fork();
    if (pid == -1)
    {
        std::cerr << "Error forking process" << std::endl;
        return;
    }
    if (pid == 0)
    { 
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        char *argv[] = { NULL, (char *)scriptPath.c_str(), NULL};
        if (scriptPath.find(".php") != std::string::npos)
            argv[0] = (char *)"/usr/bin/php-cgi";
        else if (scriptPath.find(".py") != std::string::npos)
            argv[0] = (char *)"/usr/bin/python3";
        std::string scriptFilename = "SCRIPT_FILENAME=" + scriptPath;
        std::string requestMethod = "REQUEST_METHOD=" + method;
        std::string redirectStatus = "REDIRECT_STATUS=200";
        std::string contentLength = "CONTENT_LENGTH=" + numberToString(body.size());
        char *envp[] =
        {
            (char *)scriptFilename.c_str(),
            (char *)requestMethod.c_str(),
            (char *)redirectStatus.c_str(),
            NULL
        };
        if (method == "POST")
        {
            int postPipe[2];
            pipe(postPipe);
            write(postPipe[1], body.c_str(), body.size());
            close(postPipe[1]);

            dup2(postPipe[0], STDIN_FILENO);
            close(postPipe[0]);
        }
        execve(argv[0], argv, envp);
        std::cerr << "Error executing script" << std::endl;
        exit(1);
    } 
    else
    { 
        close(pipefd[1]);   
        char buffer[1024];
        std::string httpResponse = "HTTP/1.1 200 OK\r\n";
        std::stringstream response;
        ssize_t bytesRead;
        while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0)
        {
            buffer[bytesRead] = '\0';
            response << buffer;
        }
        close(pipefd[0]);
        waitpid(pid, NULL, 0);
        std::string cgiOutput = response.str();
        size_t pos = cgiOutput.find("Content-Type:");
        if (pos != std::string::npos)
        {
            size_t end = cgiOutput.find("\r\n", pos);
            std::string contentType = cgiOutput.substr(pos, end - pos + 2);
            httpResponse += contentType;
            cgiOutput = cgiOutput.substr(end + 2);
        }
        else
            httpResponse += "Content-Type: text/html\r\n";
        httpResponse += "Content-Length: " + numberToString(response.str().size()) + "\r\n\r\n";
        httpResponse += response.str();
        send(clientFd, httpResponse.c_str(), httpResponse.size(), 0);
    }
}

void Server::answerClientEvent(int clientFd, ssize_t bytesReceived, char *buffer)
{
    std::string rawRequest(buffer, bytesReceived);
    HttpRequestParser parser;
    try
    {
        parser.parseRequest(rawRequest);
        std::cout << "Request parsed successfully!" << std::endl;
        std::cout << "Method: " << parser.getMethod() << std::endl;
        std::cout << "Path: " << _root + parser.getPath() << std::endl;
        std::string filePath = _root + parser.getPath();
        size_t pos = filePath.find("?");
        if (pos != std::string::npos)
            filePath = filePath.substr(0, pos);
        std::string fileType = parser.getType();
        if (parser.ToString(parser.getMethod()) == "POST" && parser.getPath() == "/upload")
        {
            handleFileUpload(clientFd, rawRequest);
            return;
        }
        if (parser.getPath().find(".php") != std::string::npos || parser.getPath().find(".py") != std::string::npos)
        {
            executeCGI(clientFd, filePath, parser.ToString(parser.getMethod()), parser.getBody());
            return;
        }
        if (parser.getPath() == "/" )
            filePath = _root + "/" + _index;
        else if (filePath[parser.getPath().size() - 1] == '/')
        {
            handleFoldersRequests(clientFd, parser.getPath(), fileType);
            return ;
        }
        else if (fileType == "text/html")
            filePath = _root + "/html"+parser.getPath();   
        else
            filePath = _root + parser.getPath();
        std::ifstream file(filePath.c_str());
        std::cout << "filepath" << filePath << std::endl;
        if (file.is_open())
        {
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string content = buffer.str();
            file.close();
            std::string response = "HTTP/1.1 200 OK\r\n";
            response += "Content-Type: "+ fileType + "\r\n";
            response += "Content-Length: " + numberToString(content.size()) + "\r\n";
            response += "\r\n";
            response += content;
            if (send(clientFd, response.c_str(), response.size(), 0) == -1)
                std::cerr << "Error sending message to client (fd: " << clientFd << "): " << strerror(errno) << std::endl;

        }
        else
            handleErrors(clientFd, 404);
            
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error parsing request: " << e.what() << std::endl;
        std::string response = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
        send(clientFd, response.c_str(), response.size(), 0);
    }
}

void Server::handleClientEvent(int clientFd)
{
    char buffer[4096] = {0};
    ssize_t bytesReceived = recv(clientFd, buffer, sizeof(buffer), 0);

    if (bytesReceived <= 0)
    {
        if (bytesReceived == 0)
            std::cout << "Client disconnected (fd: " << clientFd << ")" << std::endl;
        else
            std::cerr << "Error receiving message from client (fd: " << clientFd << "): " << strerror(errno) << std::endl;
        close(clientFd);
        epoll_ctl(_epollFd, EPOLL_CTL_DEL, clientFd, NULL);
    }
    else
        answerClientEvent(clientFd, bytesReceived, buffer);
        
}


void Server::run()
{

    std::cout << "Server is running..." << std::endl;
    const int MAX_EVENTS = 10;
    struct epoll_event events[MAX_EVENTS];
    while (_running)
    {
        int nfds = epoll_wait(_epollFd, events, MAX_EVENTS, 0);
        if (nfds == -1)
        {
            std::cerr << "Error in epoll_wait: " << strerror(errno) << std::endl;
            continue;
        }
        for (int i = 0; i < nfds; ++i)
        {
            if (events[i].data.fd == _serverFd)
            {
                sockaddr_in clientAddress;
                try
                {
                     acceptClient(clientAddress);
                }   
                catch (const ServerException &e)
                {
                    std::cerr << "Error accepting client: " << e.what() << std::endl;
                    continue;
                }
            }
            else
                handleClientEvent(events[i].data.fd);
        }
    }

}

void Server::stop()
{
    if (_serverFd != -1)
    {
        close(_serverFd);
        _serverFd = -1;
        _running = false;
        std::cout << "Server stopped." << std::endl;
    }
}

void Server::print() const
{
    std::cout << "=== Server Configuration ===" << std::endl;
    std::cout << "Server Name: " << (_serverName.empty() ? "Not set (default: localhost)" : _serverName) << std::endl;
    std::cout << "Port: " << _port << std::endl;
    std::cout << "Root Directory: " << (_root.empty() ? "Not set (default: ./)" : _root) << std::endl;

    if (_clientMaxBodySize != 0) {
        std::cout << "Client Max Body Size: " << _clientMaxBodySize << " bytes" << std::endl;
    } else {
        std::cout << "Client Max Body Size: Not set (default: 1048576 bytes)" << std::endl;
    }

    std::cout << "Default Index: " << (_index.empty() ? "Not set (default: index.html)" : _index) << std::endl;
    std::cout << "Autoindex: " << (_autoindex ? "Enabled" : "Disabled") << std::endl;

    std::cout << "Error Pages: " << std::endl;
    if (_errorPages.empty()) {
        std::cout << "  None configured." << std::endl;
    } else {
        for (std::map<int, std::string>::const_iterator it = _errorPages.begin(); it != _errorPages.end(); ++it) {
            std::cout << "  " << it->first << ": " << it->second << std::endl;
        }
    }

    std::cout << "Locations: " << std::endl;
    if (_locations.empty()) {
        std::cout << "  None configured." << std::endl;
    } else {
        for (size_t i = 0; i < _locations.size(); ++i) {
            std::cout << "  Location " << i + 1 << ":" << std::endl;
            _locations[i].print();
        }
    }
    std::cout << "=============================" << std::endl;
}

std::string Server::numberToString(int number)
{
    std::stringstream ss;
    ss << number;
    return ss.str();
}

std::string Server::normalizePath(const std::string &path)
{
    if (!path.empty() && path[path.size() - 1] == '/')
        return path.substr(0, path.size() - 1);
    return path;
}

ServerException::ServerException(const std::string &message): _message(message){}
const char *ServerException::what() const throw()
{
    return _message.c_str();
}

 ServerException::~ServerException() throw(){}