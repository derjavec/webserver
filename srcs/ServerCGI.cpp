#include "ServerCGI.hpp"

std::string ServerCGI::getScriptExecutor(const std::string &fileExtension)
{
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

char **ServerCGI::setupCGIEnvironment(const std::string &scriptPath, const std::string &method, const std::string &body)
{
    std::string scriptFilename = "SCRIPT_FILENAME=" + scriptPath;
    std::string requestMethod = "REQUEST_METHOD=" + method;
    std::string redirectStatus = "REDIRECT_STATUS=200";
    std::string contentLength = "CONTENT_LENGTH=" + ServerUtils::numberToString(body.size());
    std::string contentType = "CONTENT_TYPE=application/x-www-form-urlencoded";
    std::string gatewayInterface = "GATEWAY_INTERFACE=CGI/1.1";
    std::string queryString = "QUERY_STRING=";

    char **envp = new char *[7];
    envp[0] = strdup(scriptFilename.c_str());
    envp[1] = strdup(requestMethod.c_str());
    envp[2] = strdup(redirectStatus.c_str());
    envp[3] = strdup(contentLength.c_str());
    envp[4] = strdup(contentType.c_str());
    envp[5] = strdup(gatewayInterface.c_str());
    envp[6] = NULL;


    return envp;
}

void ServerCGI::handleCGIChildProcess(int pipefd[2], const std::string &scriptExecutor, const std::string &scriptPath,const std::string &method, const std::string &body)
{
    dup2(pipefd[1], STDOUT_FILENO);
    close(pipefd[0]);
    close(pipefd[1]);
    char *argv[] = {(char *)scriptExecutor.c_str(), (char *)scriptPath.c_str(), NULL};
    char **envp = setupCGIEnvironment(scriptPath, method, body);
    if (method == "POST")
    {
        int postPipe[2];
        if (pipe(postPipe) == -1)
        {
            std::cerr << "Error creating pipe for POST body" << std::endl;
            exit(1);
        }
        if (write(postPipe[1], body.c_str(), body.size()) == -1)
        {
            std::cerr << "Error writing POST body to pipe" << std::endl;
            exit(1);
        }   
        close(postPipe[1]);
        dup2(postPipe[0], STDIN_FILENO);
        close(postPipe[0]);
    }
    execve(argv[0], argv, envp);
    std::cerr << "Error executing script: " << scriptPath << std::endl;
    exit(1);
}

void ServerCGI::handleCGIParentProcess(Server &server, int clientFd, int pipefd[2], pid_t pid)
{
	
	close(pipefd[1]);
	FDStreamBuf fdBuf(pipefd[0], 4046);
	std::istream is(&fdBuf);
	std::stringstream response;
	response << is.rdbuf();
	waitpid(pid, NULL, 0); 
	close(pipefd[0]);
	std::string cgiOutput = response.str();
	std::string httpResponse = "HTTP/1.1 200 OK\r\n";
	httpResponse += ServerUtils::buildStandardHeaders();
	
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
    httpResponse += "Content-Length: " + ServerUtils::numberToString(cgiOutput.size()) + "\r\n";
    httpResponse += "Set-Cookie: session_id=" + server._clientSessions[clientFd].sessionId + "; Path=/; HttpOnly\r\n";
    httpResponse += "\r\n";
    httpResponse += cgiOutput;
    send(clientFd, httpResponse.c_str(), httpResponse.size(), 0);
    shutdown(clientFd, SHUT_WR);
    close(clientFd);
}

void ServerCGI::executeCGI(Server &server, int clientFd, std::string &filePath, HttpRequestParser &parser)
{
    size_t dotPos = filePath.find_last_of(".");
    std::string fileExtension = (dotPos != std::string::npos) ? filePath.substr(dotPos + 1) : "";
    std::string scriptExecutor = getScriptExecutor(fileExtension);
    std::vector<std::string> cgiPaths;   
    std::vector<std::string> cgiExt;
    std::string url = parser.getPath();
    std::cout << url << std::endl;
    if (ResolvePaths::isLocation(server, url) && !getCGIdataInLoc(server, url, cgiPaths, cgiExt))
    {
        bool validExtension = cgiExt.empty() || (std::find(cgiExt.begin(), cgiExt.end(), fileExtension) != cgiExt.end());
        if (cgiPaths.empty() || !validExtension)
        {
            std::cerr << "❌ CGI execution not allowed for: " << filePath << std::endl;
            ServerErrors::handleErrors(server, clientFd, 403);
            return;
        }
        scriptExecutor = cgiPaths[0]; 
    }
    if (scriptExecutor.empty())
    {
        std::cerr << "No script executor found for: " << filePath << std::endl;
        return;
    }
    if (access(filePath.c_str(), F_OK) != 0)
    {
        std::cerr << "❌ Error: CGI script not found: " << filePath << std::endl;
        std::string response = "HTTP/1.1 404 Not Found\r\n";
        response += ServerUtils::buildStandardHeaders();
        response += "Content-Type: text/html\r\n";
        response += "Content-Length: 74\r\n\r\n";
        response += "<html><body><h1>404 Not Found</h1><p>CGI script not found.</p></body></html>";
        send(clientFd, response.c_str(), response.size(), 0);
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
    std::string method = parser.methodToString(parser.getMethod());
    std::string body = parser.getBody();
    if (pid == 0)
        handleCGIChildProcess(pipefd, scriptExecutor, filePath, method, body);
    else
        handleCGIParentProcess(server, clientFd, pipefd, pid);
}

bool ServerCGI::hasCGIPassInLoc(Server &server, std::string url)
{
    size_t bestMatchLength = 0;
    const LocationConfig* bestLocation = ResolvePaths::findBestLocation(server, url, bestMatchLength);

    if (bestLocation)
    {
        const std::vector<std::string>& cgiPaths = bestLocation->getCgiPath();
        return !cgiPaths.empty();
    }
    return false;
}

bool ServerCGI::getCGIdataInLoc(Server &server, std::string url, std::vector<std::string>& cgiPaths, std::vector<std::string>& cgiExt)
{
    size_t bestMatchLength = 0;
    const LocationConfig* bestLocation = ResolvePaths::findBestLocation(server, url, bestMatchLength);

    if (!bestLocation)
        return false;
    cgiPaths = bestLocation->getCgiPath();
    cgiExt = bestLocation->getCgiExt();
    if (cgiPaths.empty())
        return false;
    size_t dotPos = url.find_last_of(".");
    if (dotPos == std::string::npos)
        return false;
    std::string fileExtension = url.substr(dotPos);
    bool validExtension = false;
    for (size_t i = 0; i < cgiExt.size(); i++)
    {
        if (fileExtension == cgiExt[i])
        {
            validExtension = true;
            break;
        }
    }
    if (!validExtension)
    {
        std::cerr << "❌ CGI execution not allowed for this extension: " << fileExtension << std::endl;
        return false;
    }
    return true;
}

