#include "ServerUtils.hpp"
#include "Server.hpp"
#include "FDStreamBuf.hpp"

std::map<int, ClientState> clientStates;

bool directoryExists(const std::string &path)
{
    struct stat info;
    return (stat(path.c_str(), &info) == 0 && (info.st_mode & S_IFDIR));
}

std::string ServerUtils::resolveFilePath(Server &server, HttpRequestParser parser)
{
    std::string requestedPath = parser.getPath();
    size_t pos = requestedPath.find("?");
    std::string cleanPath;
    if (pos != std::string::npos)
	   cleanPath = requestedPath.substr(0, pos);
    else
        cleanPath = requestedPath;
    std::pair<std::string, std::string> fileInfo = parser.getContentType(cleanPath);
    std::string fileCategory = fileInfo.second;
   // if (cleanPath.find("/upload") == 0)
    	//cleanPath = server._root + cleanPath;
    if (cleanPath.find(server._root) != 0)  
    {
        if (fileCategory == "html")
            cleanPath = server._root + "/html" + cleanPath;
        else
            cleanPath = server._root + cleanPath;
    }
    if (cleanPath.substr(0, 2) == "./")
        cleanPath = cleanPath.substr(2);
    return cleanPath;
}

UploadData ServerUtils::parseFileUploadRequest(const std::string& request)
{
    UploadData data;
    size_t boundaryPos = request.find("boundary=");
    if (boundaryPos == std::string::npos)
    {
        std::cerr << "No boundary found in request" << std::endl;
        return data;
    }
    std::string boundaryValue = request.substr(boundaryPos + 9);
    size_t endBoundary = boundaryValue.find_first_of("\r\n ");
    if (endBoundary != std::string::npos)
        boundaryValue = boundaryValue.substr(0, endBoundary);
    data.boundary = "--" + boundaryValue;
    size_t cdPos = request.find("Content-Disposition:");
    if (cdPos == std::string::npos)
    {
        std::cerr << "No Content-Disposition header found" << std::endl;
        return data;
    }
    size_t headerEnd = request.find("\r\n\r\n", cdPos);
    if (headerEnd == std::string::npos)
    {
        std::cerr << "Malformed multipart header" << std::endl;
        return data;
    }
    std::string headerBlock = request.substr(cdPos, headerEnd - cdPos);
    size_t filenamePos = headerBlock.find("filename=\"");
    if (filenamePos == std::string::npos)
    {
        std::cerr << "No filename found in Content-Disposition" << std::endl;
        return data;
    }
    filenamePos += 10; 
    size_t filenameEnd = headerBlock.find("\"", filenamePos);
    if (filenameEnd == std::string::npos)
    {
        std::cerr << "Filename not terminated properly" << std::endl;
        return data;
    }
    data.filename = headerBlock.substr(filenamePos, filenameEnd - filenamePos);
    std::cout << "Extracted filename: " << data.filename << std::endl;
    size_t fileStart = headerEnd + 4;
    size_t fileEnd = request.find(data.boundary, fileStart);
    if (fileEnd == std::string::npos)
    {
        std::cerr << "No ending boundary found for file content" << std::endl;
        return data;
    }
    while (fileEnd > fileStart && (request[fileEnd - 1] == '\r' || request[fileEnd - 1] == '\n'))
        --fileEnd;
    data.fileContent = request.substr(fileStart, fileEnd - fileStart);
    return data;
}


void ServerUtils::handleFileUpload(Server &server, int clientFd, const std::string& request)
{
    UploadData data = parseFileUploadRequest(request);

    if (data.filename.empty() || data.fileContent.empty())
    {
        std::cerr << "Invalid upload request" << std::endl;
        handleErrors(server, clientFd, 400);
        return;
    }
    bool uploadAllowed = false;
    for (std::vector<LocationConfig>::const_iterator it = server._locations.begin(); it != server._locations.end(); ++it) {
        if (normalizePath(it->getPath()) == "/upload")
        {
            uploadAllowed = true;
            std::string uploadDir = "www/upload/";
            struct stat sb;
            if (stat(uploadDir.c_str(), &sb) != 0 || !(sb.st_mode & S_IFDIR))
            {
                std::cerr << "Upload directory does not exist: " << uploadDir << std::endl;
                handleErrors(server, clientFd, 500);
                return;
            }
            std::string filePath = uploadDir + data.filename;
            std::ofstream outFile(filePath.c_str(), std::ios::binary);
            if (!outFile)
            {
                std::cerr << "Error opening file for writing" << std::endl;
                handleErrors(server, clientFd, 500);
                return;
            }
            outFile.write(data.fileContent.c_str(), data.fileContent.size());
            outFile.close();
            break;
        }
    }
    if (!uploadAllowed)
    {
        std::cerr << "Upload location not allowed" << std::endl;
        handleErrors(server, clientFd, 403);
        return;
    }
    std::string body = "<html><body><h1>Upload Successful</h1><p>File uploaded: " + data.filename + "</p></body></html>";
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/html\r\n";
    response += "Content-Length: " + numberToString(body.size()) + "\r\n\r\n";
    response += body;
    send(clientFd, response.c_str(), response.size(), 0);
    shutdown(clientFd, SHUT_WR);
    close(clientFd);
}

bool ServerUtils::findLocationConfig(Server &server, const std::string &path, std::string &locationIndex, bool &autoindexEnabled, std::string &filePath)
{
    for (std::vector<LocationConfig>::const_iterator it = server._locations.begin(); it != server._locations.end(); ++it)
    {
        if (normalizePath(path) == normalizePath(it->getPath()))
        {
            locationIndex = it->getIndex();
            autoindexEnabled = it->isAutoindexEnabled();
            filePath = it->getRoot() + "/" +locationIndex;
            std::cout <<" encuentra la location :"<< locationIndex << " " << autoindexEnabled << " " << filePath << std::endl;
            return true;
        }
    }
    return false;
}

std::string ServerUtils::extractPathFromURL(const std::string &url)
{
    size_t pos = url.find("://");
    if (pos != std::string::npos)
    {
        pos = url.find('/', pos + 3);
        if (pos != std::string::npos)
            return url.substr(pos);
    }
    return url;
}

void ServerUtils::handleRedirections(Server &server, HttpRequestParser &parser, std::string &externalRedirect)
{
    for (std::vector<LocationConfig>::const_iterator it = server._locations.begin(); it != server._locations.end(); ++it)
    {
        if (normalizePath(parser.getPath()) == normalizePath(it->getPath()) && !it->getRedirect().empty())
        {
            const std::string &redirectTarget = extractPathFromURL(it->getRedirect());
            for (std::vector<LocationConfig>::const_iterator locIt = server._locations.begin(); locIt != server._locations.end(); ++locIt)
            {
                if (normalizePath(redirectTarget) == normalizePath(locIt->getPath()))
                {
                    std::cout << "Internal redirect to location: " << redirectTarget << std::endl;
                    parser.setPath(redirectTarget);
                    parser.setType(redirectTarget);
                    return;
                }
            }
            externalRedirect = redirectTarget;
            return;
        }
    }
    return;
}

void ServerUtils::handleFoldersRequests(Server &server, int clientFd, const std::string &path, const std::string &fileType)
{
    std::string locationIndex;
    bool autoindexEnabled = false;
    std::string filePath;

    if (!findLocationConfig(server, path, locationIndex, autoindexEnabled, filePath))
    {
        handleErrors(server, clientFd, 404);
        return;
    }
    if (locationIndex.empty())
    {
        handleAutoIndex(server, clientFd, autoindexEnabled, path);
        return;
    }
    std::ifstream file(filePath.c_str());
    if (file.is_open())
    {
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        file.close();

        std::string response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: " + fileType + "\r\n";
        response += "Content-Length: " + numberToString(content.size()) + "\r\n";
        response += "\r\n";
        response += content;

        if (send(clientFd, response.c_str(), response.size(), 0) == -1)
            std::cerr << "Error sending message to client (fd: " << clientFd << "): " << strerror(errno) << std::endl;
    }
}

void ServerUtils::handleClientWrite(Server &server, int clientFd)
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

void ServerUtils::serveLargeFileAsync(Server &server, int clientFd, const std::string& filePath, const std::string &ContentType)
{
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
                           "Content-Type:" + ContentType + "\r\n"
                           "Content-Length: " + numberToString(fileStat.st_size) + "\r\n\r\n";
    send(clientFd, response.c_str(), response.size(), 0);

    clientStates[clientFd] = ClientState();
    clientStates[clientFd].fileFd = fileFd;
    clientStates[clientFd].offset = 0;
    clientStates[clientFd].totalSize = fileStat.st_size;
    handleClientWrite(server, clientFd);
}

void ServerUtils::serveStaticFile(Server &server, int clientFd, const std::string &filePath, const std::string &contentType)
{
    std::ifstream file(filePath.c_str(), std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Error: Couldn't open the file " << filePath << std::endl;
        handleErrors(server, clientFd, 404);
        return;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: " + contentType + "\r\n";
    response += "Content-Length: " + numberToString(content.size()) + "\r\n";
    response += "Connection: close\r\n\r\n";
    response += content;
    if (send(clientFd, response.c_str(), response.size(), 0) == -1)
    {
        std::cerr << "Error esending file to client (fd: " << clientFd << "): " << strerror(errno) << std::endl;
    }
}

std::string ServerUtils::getScriptExecutor(const std::string &fileExtension)
{
    std::cout << "file extension : "<< fileExtension << std::endl;
    static std::map<std::string, std::string> scriptExecutors;
    if (scriptExecutors.empty())
    {
        scriptExecutors["php"] = "/usr/bin/php-cgi";
        scriptExecutors["py"] = "/usr/bin/python3";
        scriptExecutors["sh"] = "/bin/bash"; 
        scriptExecutors["pl"] = "/usr/bin/perl";
        scriptExecutors["rb"] = "/usr/bin/ruby";
        scriptExecutors["cpp"] = "/usr/bin/g++";
        scriptExecutors["h"] = "/usr/bin/gcc";
        scriptExecutors["java"] = "/usr/bin/java";
    }
    std::map<std::string, std::string>::const_iterator it = scriptExecutors.find(fileExtension);
    if (it != scriptExecutors.end())
        return it->second;
    return ""; 
}

char **ServerUtils::setupCGIEnvironment(const std::string &scriptPath, const std::string &method, const std::string &body)
{
    std::string scriptFilename = "SCRIPT_FILENAME=" + scriptPath;
    std::string requestMethod = "REQUEST_METHOD=" + method;
    std::string redirectStatus = "REDIRECT_STATUS=200";
    std::string contentLength = "CONTENT_LENGTH=" + numberToString(body.size());

    char **envp = new char *[5];
    envp[0] = strdup(scriptFilename.c_str());
    envp[1] = strdup(requestMethod.c_str());
    envp[2] = strdup(redirectStatus.c_str());
    envp[3] = strdup(contentLength.c_str());
    envp[4] = NULL;

    return envp;
}

void ServerUtils::handleCGIChildProcess(int pipefd[2], const std::string &scriptExecutor,
		const std::string &scriptPath,const std::string &method, const std::string &body)
{
    dup2(pipefd[1], STDOUT_FILENO);
    close(pipefd[0]);
    close(pipefd[1]);
    char *argv[] = {(char *)scriptExecutor.c_str(), (char *)scriptPath.c_str(), NULL};
    char **envp = setupCGIEnvironment(scriptPath, method, body);
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
    std::cerr << "Error executing script: " << scriptPath << std::endl;
    exit(1);
}

void ServerUtils::handleCGIParentProcess(int clientFd, int pipefd[2], pid_t pid)
{
	
 std::cerr << "[handleCGIParentProcess] Closing pipefd[1]" << std::endl;	 
	close(pipefd[1]);
	FDStreamBuf fdBuf(pipefd[0], 4046);
	std::istream is(&fdBuf);
	std::stringstream response;
	response << is.rdbuf();
	waitpid(pid, NULL, 0); 
	  close(pipefd[0]);
	std::string cgiOutput = response.str();
	std::string httpResponse = "HTTP/1.1 200 OK\r\n";
	size_t pos = cgiOutput.find("Content-Type:");
    if (pos != std::string::npos)
    {
        size_t end = cgiOutput.find("\n", pos);
        if (end == std::string::npos)
            end = cgiOutput.find("\r\n", pos);

        std::string contentType = cgiOutput.substr(pos, end - pos + 1);
        httpResponse += contentType + "\r\n";
        cgiOutput = cgiOutput.substr(end + 1);
    }
    else
        httpResponse += "Content-Type: text/html\r\n";
    httpResponse += "Content-Length: " + numberToString(cgiOutput.size()) + "\r\n";
    httpResponse += "\r\n";
    httpResponse += cgiOutput;
    send(clientFd, httpResponse.c_str(), httpResponse.size(), 0);
    shutdown(clientFd, SHUT_WR);
    close(clientFd);
}

void ServerUtils::executeCGI(int clientFd, const std::string &scriptPath, const std::string &method,const std::string &body, const std::string &scriptExecutor)
{
    if (scriptExecutor.empty())
    {
        std::cerr << "No script executor found for: " << scriptPath << std::endl;
        return;
    }
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
        handleCGIChildProcess(pipefd, scriptExecutor, scriptPath, method, body);
    else
        handleCGIParentProcess(clientFd, pipefd, pid);
}

void ServerUtils::handleAutoIndex(Server &server, int clientFd, bool autoindexEnabled, std::string path)
{
    if (autoindexEnabled)
    {
        std::string directoryListing = generateAutoindexPage(server, path);
        std::string response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: text/html\r\n";
        response += "Content-Length: " + numberToString(directoryListing.size()) + "\r\n";
        response += "\r\n";
        response += directoryListing;
        std::cout << "respuesta enviada: "<< response << std::endl;
        send(clientFd, response.c_str(), response.size(), 0);
    }
    else
        handleErrors(server, clientFd, 403);
    return ;
}

std::string ServerUtils::generateAutoindexPage(Server &server, const std::string& directoryPath)
{
    std::string absolutePath = server._root + directoryPath;
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

void ServerUtils::handleDeleteRequest(Server &server, int clientFd, HttpRequestParser &parser)
{
    std::string filePath = resolveFilePath(server, parser);
    std::cout << "DELETE request for file: " << filePath << std::endl;
    if (access(filePath.c_str(), F_OK) != 0)
    {
        handleErrors(server, clientFd, 404);
        return;
    }
    if (remove(filePath.c_str()) == 0)
    {
        std::string response = "HTTP/1.1 200 OK\r\n"
                               "Content-Type: text/html\r\n"
                               "Content-Length: 37\r\n\r\n"
                               "File successfully deleted.";
        if (send(clientFd, response.c_str(), response.size(), 0) == -1)
            std::cerr << "Error sending DELETE response: " << strerror(errno) << std::endl;

    }
    else
        handleErrors(server, clientFd, 500);

    shutdown(clientFd, SHUT_WR);
    close(clientFd);
}

size_t ServerUtils::getFileSize(const std::string &filePath)
{
    struct stat fileStat;
    if (stat(filePath.c_str(), &fileStat) == 0)
        return fileStat.st_size;
    return 0;
}

void ServerUtils::sendGenericErrorResponse(int clientFd, int code)
{
    std::string response = "HTTP/1.1 " + numberToString(code) + " " + getStatusMessage(code) + "\r\n";
    std::string genericContent = "<html><body><h1>" + numberToString(code) + " " + getStatusMessage(code) + "</h1></body></html>";
    
    response += "Content-Type: text/html\r\n";
    response += "Content-Length: " + numberToString(genericContent.size()) + "\r\n\r\n";
    response += genericContent;
    if (send(clientFd, response.c_str(), response.size(), 0) == -1)
        std::cerr << "Error sending generic " << code << " response to client (fd: " << clientFd << "): " << strerror(errno) << std::endl;
}

void ServerUtils::handleErrors(Server &server, int clientFd, int code)
{
    std::map<int, std::string>::const_iterator it = server._errorPages.find(code);
    if (it == server._errorPages.end())
    {
        std::cerr << "⚠️ Error code not found in configuration: " << code << std::endl;
        sendGenericErrorResponse(clientFd, code);
        return;
    }
    std::string errorFilePath = server._root + it->second;
    std::ifstream errorFile(errorFilePath.c_str());
    if (errorFile.is_open())
    {
        std::stringstream errorBuffer;
        errorBuffer << errorFile.rdbuf();
        std::string errorContent = errorBuffer.str();
        errorFile.close();

        std::string response = "HTTP/1.1 " + numberToString(code) + " " + getStatusMessage(code) + "\r\n";
        response += "Content-Type: text/html\r\n";
        response += "Content-Length: " + numberToString(errorContent.size()) + "\r\n\r\n";
        response += errorContent;

        if (send(clientFd, response.c_str(), response.size(), 0) == -1)
            std::cerr << " Error sending " << code << " error page to client (fd: " << clientFd << "): " << strerror(errno) << std::endl;
    }
    else
    {
        std::cerr << "⚠️ Error page not found: " << errorFilePath << std::endl;
        sendGenericErrorResponse(clientFd, code);
    }
    shutdown(clientFd, SHUT_WR);
    close(clientFd);
}

std::string ServerUtils::getStatusMessage(int code)
{
    switch (code)
    {
	case 400: return "Bas Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
	case 405: return "Method Not Allowed";
	case 413: return "Request Entity Too Large";
        case 500: return "Internal Server Error";
	case 501: return "Not Implemented";
	case 505: return "HTTP Version Not Supported";
        default: return "Error";
    }
}

std::string ServerUtils::numberToString(int number)
{
    std::stringstream ss;
    ss << number;
    return ss.str();
}

std::string ServerUtils::normalizePath(const std::string &path)
{
    if (!path.empty() && path[path.size() - 1] == '/')
        return path.substr(0, path.size() - 1);
    return path;
}







