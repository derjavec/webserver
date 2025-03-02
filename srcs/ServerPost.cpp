#include "ServerPost.hpp"

UploadData ServerPost::parseFileUploadRequest(const std::vector<char>& requestBuffer)
{
    UploadData data;

    std::string requestStr(requestBuffer.begin(), requestBuffer.end());
    std::string boundaryPrefix = "boundary=";
    size_t boundaryPos = requestStr.find(boundaryPrefix);
    if (boundaryPos == std::string::npos)
    {
        std::cerr << "❌ No boundary found in request" << std::endl;
        return data;
    }
    boundaryPos += boundaryPrefix.size();
    size_t boundaryEnd = requestStr.find("\r\n", boundaryPos);
    if (boundaryEnd == std::string::npos)
    {
        std::cerr << "❌ Boundary not terminated properly" << std::endl;
        return data;
    }
    data.boundary = "--" + requestStr.substr(boundaryPos, boundaryEnd - boundaryPos);
    std::string cdMarker = "Content-Disposition:";
    size_t cdPos = requestStr.find(cdMarker);
    if (cdPos == std::string::npos)
    {
        std::cerr << "❌ No Content-Disposition header found" << std::endl;
        return data;
    }
    std::string filenameMarker = "filename=\"";
    size_t filenamePos = requestStr.find(filenameMarker, cdPos);
    if (filenamePos == std::string::npos)
    {
        std::cerr << "❌ No filename found in Content-Disposition" << std::endl;
        return data;
    }
    filenamePos += filenameMarker.size();
    size_t filenameEnd = requestStr.find("\"", filenamePos);
    if (filenameEnd == std::string::npos)
    {
        std::cerr << "❌ Filename not terminated properly" << std::endl;
        return data;
    }
    data.filename = requestStr.substr(filenamePos, filenameEnd - filenamePos);
    std::string headerEndMarker = "\r\n\r\n";
    size_t contentStartPos = requestStr.find(headerEndMarker, filenameEnd);
    if (contentStartPos == std::string::npos)
    {
        std::cerr << "❌ Malformed multipart header" << std::endl;
        return data;
    }
    contentStartPos += headerEndMarker.size();
    size_t contentEndPos = requestStr.find(data.boundary, contentStartPos);
    if (contentEndPos == std::string::npos)
    {
        std::cerr << "❌ No ending boundary found for file content" << std::endl;
        return data;
    }
    while (contentEndPos > contentStartPos && (requestStr[contentEndPos - 1] == '\r' || requestStr[contentEndPos - 1] == '\n'))
        --contentEndPos;
    data.fileContent.assign(requestBuffer.begin() + contentStartPos, requestBuffer.begin() + contentEndPos);
    return data;
}


void ServerPost::handleBigFileUpload(Server &server, int clientFd, UploadData data, std::string filePath)
{
    std::ofstream outFile(filePath.c_str(), std::ios::binary | std::ios::app);
    if (!outFile)
    {
        std::cerr << "❌ Error opening file for writing" << std::endl;
        ServerErrors::handleErrors(server, clientFd, 500);
        return;
    }
    size_t offset = 0;
    const size_t chunkSize = 8192;
    while (offset < data.fileContent.size())
    {
        size_t writeSize = std::min(chunkSize, data.fileContent.size() - offset);
        outFile.write(data.fileContent.c_str() + offset, writeSize);
        offset += writeSize;
    }
    outFile.close();
}

void ServerPost::handleSmallFileUpload(Server &server, int clientFd, UploadData data, std::string filePath)
{
        std::ofstream outFile(filePath.c_str(), std::ios::binary);
        if (!outFile)
        {
            std::cerr << "Error opening file for writing" << std::endl;
            ServerErrors::handleErrors(server, clientFd, 500);
            return;
        }
        outFile.write(data.fileContent.c_str(), data.fileContent.size());
        outFile.close();
        return;
}

void ServerPost::handleFileUpload(Server &server, int clientFd, const std::vector<char>& requestBuffer)
{
    UploadData data = parseFileUploadRequest(requestBuffer);
    std::string uploadDir = server._upload;
    std::cout<< "data :"<<data.filename<< " "<<data.fileContent<<std::endl;
    if (data.filename.empty() || data.fileContent.empty())
    {
        std::cerr << "❌ Invalid upload request" << std::endl;
        ServerErrors::handleErrors(server, clientFd, 400);
        return;
    }
    std::string filePath = uploadDir + "/" + data.filename;
    std::ifstream file(filePath.c_str());
    if (file.good())
    {
        file.close();
        std::cerr << "⚠️ File already exists: " << filePath << std::endl;
        std::string body = "<html><body><h1>File Already Exists</h1><p>File: " + data.filename + " already uploaded.</p></body></html>";
        std::string response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: text/html\r\n";
        response += "Content-Length: " + ServerUtils::numberToString(body.size()) + "\r\n\r\n";
        response += body;
        send(clientFd, response.c_str(), response.size(), 0);
        shutdown(clientFd, SHUT_WR);
        close(clientFd);
        return;
    }
    file.close();
    if (data.fileContent.size() > server._clientMaxBodySize)
        handleBigFileUpload(server, clientFd, data, filePath);
    else
        handleSmallFileUpload(server, clientFd, data, filePath);      
    std::string body = "<html><body><h1>Upload Successful</h1><p>File uploaded: " + data.filename + "</p></body></html>";
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/html\r\n";
    response += "Content-Length: " + ServerUtils::numberToString(body.size()) + "\r\n\r\n";
    response += body;
    send(clientFd, response.c_str(), response.size(), 0);
    shutdown(clientFd, SHUT_WR);
    close(clientFd);
}

std::string ServerPost::getRootFromForm(Server &server)
{
    std::string formRoot;

    for (size_t i = 0; i < server._locations.size(); ++i)
    {
        if (server._locations[i].getPath() == "/form")  
        {
            formRoot = server._locations[i].getRoot();
            return (formRoot);
        }
    }
    return "";
}

bool ServerPost::extractFormData(const std::vector<char>& clientBuffer, std::string& formData)
{
    std::string requestStr(clientBuffer.begin(), clientBuffer.end());
    size_t headerEndPos = requestStr.find("\r\n\r\n");
    
    if (headerEndPos == std::string::npos)
    {
        std::cerr << "❌ Malformed request: No header-body separator found" << std::endl;
        return false;
    }
    formData = requestStr.substr(headerEndPos + 4);
    return true;
}

bool ServerPost::getFormFilePath(Server &server, const std::string& formFileName, std::string& formFilePath)
{
    std::cout<<"name :"<<formFileName<<std::endl;
    if (formFileName.empty())
    {
        std::cerr << "❌ formFileName is empty" << std::endl;
        return false;
    }
    std::string formRoot = getRootFromForm(server);
    if (formRoot.empty())
    {
        std::cerr << "❌ No valid form root directory found for /form" << std::endl;
        return false;
    }

    formFilePath = formRoot + "/" + formFileName;
    std::cout<<"formfilepath:"<<formFilePath<<std::endl;
    return true;
}

bool ServerPost::writeFormData(const std::string& formFilePath, const std::map<std::string, std::string>& formFields, std::string sessionId)
{
    bool fileExists = (access(formFilePath.c_str(), F_OK) == 0);
    std::fstream formFile(formFilePath.c_str(), std::ios::out | std::ios::app);
    
    if (!formFile.is_open())
    {
        std::cerr << "❌ Error opening form submissions file" << std::endl;
        return false;
    }
    if (!fileExists)
    {
        for (std::map<std::string, std::string>::const_iterator it = formFields.begin(); it != formFields.end(); ++it)
        {
            if (it->first != "form_file")
                formFile << it->first << ",";
        }
        formFile << "Session ID" << ",";
        formFile.seekp(-1, std::ios_base::end);
        formFile << "\n";
    }
    for (std::map<std::string, std::string>::const_iterator it = formFields.begin(); it != formFields.end(); ++it)
    {
        if (it->first != "form_file")
            formFile << it->second << ",";
    }
    formFile.seekp(-1, std::ios_base::end);
    formFile << sessionId; 
    formFile << "\n";
    formFile.close();
    return true;
}

std::string ServerPost::verifyLogin(const std::string& filePath, const std::map<std::string, std::string>& formFields)
{
    std::ifstream file(filePath.c_str());
    if (!file.is_open())
        return "";
    std::string line;
    std::getline(file, line);

    std::vector<std::string> headers;
    std::stringstream headerStream(line);
    std::string column;
    while (std::getline(headerStream, column, ','))
        headers.push_back(column);

    while (std::getline(file, line))
    {
        std::stringstream ss(line);
        std::string value;
        std::map<std::string, std::string> rowValues;
        size_t index = 0;

        while (std::getline(ss, value, ',') && index < headers.size())
        {
            rowValues[headers[index]] = value;
            ++index;
        }

        bool match = true;
        for (std::map<std::string, std::string>::const_iterator it = formFields.begin(); it != formFields.end(); ++it)
        {
            if (it->first != "form_file" && (rowValues[it->first] != it->second))
            {
                match = false;
                break;
            }
        }
        if (match) 
            return rowValues["Session ID"];
    }
    return "";
}

std::map<std::string, std::string> ServerPost::getFormFields(Server &server, int clientFd, std::vector<char>& clientBuffer, std::string &formFilePath)
{
    std::string formData;
    if (!extractFormData(clientBuffer, formData))
    {
        ServerErrors::handleErrors(server, clientFd, 400);
        return std::map<std::string, std::string>();
    }
    std::map<std::string, std::string> formFields = ResolvePaths::parseURLEncoded(formData); 
    if (formFields.count("form_file") > 0 && !getFormFilePath(server, formFields["form_file"], formFilePath))
    {
        ServerErrors::handleErrors(server, clientFd, 500);
        return std::map<std::string, std::string>();
    }
    return formFields;
}

void ServerPost::handleFormLogin(Server &server, int clientFd, std::vector<char>& clientBuffer)
{
    std::string formFilePath;
    std::map<std::string, std::string> formFields = getFormFields(server, clientFd, clientBuffer, formFilePath);
    if (formFields.empty())
        return ;
    std::string storedSessionId = verifyLogin(formFilePath, formFields);
    std::cout << storedSessionId<<std::endl;
    if (storedSessionId.empty())
    {
        ServerErrors::handleErrors(server, clientFd, 500);
        return;
    }
    if (server._clientSessions.find(clientFd) == server._clientSessions.end())
    {
        SessionData newSession;
        newSession.sessionId = storedSessionId;
        newSession.isLoggedIn = true;
        newSession.connectionTime = std::time(NULL);
        server._clientSessions[clientFd] = newSession;
    }
    else
    {
        server._clientSessions[clientFd].sessionId = storedSessionId;
        server._clientSessions[clientFd].isLoggedIn = true;
        server._clientSessions[clientFd].connectionTime = std::time(NULL);
    }
    std::string username = formFields["username"];
    std::string response = "HTTP/1.1 302 Found\r\n";
    response += "Set-Cookie: session_id=" + storedSessionId + "; Path=/\r\n";
    response += "Location: /welcome.html?user=" + username + "\r\n";
    response += "Content-Length: 0\r\n\r\n";
    send(clientFd, response.c_str(), response.size(), 0);
    shutdown(clientFd, SHUT_WR);
    close(clientFd);
}

void ServerPost::handleFormSubmission(Server &server, int clientFd, std::vector<char>& clientBuffer, const std::string &sessionId, const std::string &path)
{
    std::string formFilePath;
    if (path == "/submit-form")
    {
        std::map<std::string, std::string> formFields = getFormFields(server, clientFd, clientBuffer, formFilePath);
        if (formFields.empty())
            return ;
        if (formFilePath == "login.csv")
        {
            std::string existingSessionId = verifyLogin(formFilePath, formFields);
            if (!existingSessionId.empty())
            {
                std::cerr << "❌ Error: User already registered!" << std::endl;
                std::string response = "HTTP/1.1 302 Conflict\r\n"
                                    "Location: /login.html\r\n"
                                    "Content-Type: text/html\r\n"
                                    "Content-Length: 67\r\n\r\n"
                                    "<html><body>User already has an account!</body></html>";
                send(clientFd, response.c_str(), response.size(), 0);
                shutdown(clientFd, SHUT_WR);
                close(clientFd);
                return;
            }
        }
        if (formFields.count("form_file") > 0 && !writeFormData(formFilePath, formFields, sessionId))
        {
            ServerErrors::handleErrors(server, clientFd, 500);
            return;
        }
    }
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Set-Cookie: session_id=" + sessionId + "; Path=/; HttpOnly\r\n";
    response += "Content-Type: text/html\r\n";
    if (formFilePath == "login.csv") 
    {
        response += "Content-Length: 124\r\n\r\n";
        response += "<html><head><meta http-equiv=\"refresh\" content=\"2;url=/login.html\"></head>"
                    "<body>Perfect! Redirecting to login...</body></html>";
    }
    else 
    {
        std::string body = "<html><body><h1>Form received successfully!</h1></body></html>";
        response += "Content-Length: " + ServerUtils::numberToString(body.size()) + "\r\n\r\n";
        response += body;
    }    
    send(clientFd, response.c_str(), response.size(), 0);
    shutdown(clientFd, SHUT_WR);
    close(clientFd);
}
    
std::string ServerPost::findUsernameBySessionId(const std::string &filePath, const std::string &sessionId)
{
    std::ifstream file(filePath.c_str());
    if (!file.is_open())
    {
        std::cerr << "❌ Error: Could not open login file: " << filePath << std::endl;
        return "";
    }
    std::string line;
    if (!std::getline(file, line)) 
    {
        std::cerr << "❌ Error: Empty login file." << std::endl;
        return "";
    }
    std::stringstream headerStream(line);
    std::vector<std::string> headers;
    std::string column;
    while (std::getline(headerStream, column, ','))
        headers.push_back(column);
    while (std::getline(file, line))
    {
        std::stringstream ss(line);
        std::string value;
        std::map<std::string,std::string> rowValues;
        size_t index = 0;
        while (std::getline(ss, value, ',') && index < headers.size())
        {
            rowValues[headers[index]] = value;
            index++;
        }
        std::string fileSessionId = ServerUtils::trim(rowValues["Session ID"]);
        if (fileSessionId == sessionId)
            return rowValues["username"];
    }  
    return "";
}


bool ServerPost::checkClientSession(Server &server, int clientFd, std::vector<char>& clientBuffer, const std::string &sessionId)
{
    std::map<int, SessionData>::iterator it = server._clientSessions.find(clientFd);
    if (it == server._clientSessions.end())
        return false;
    std::time_t currentTime = std::time(NULL);
    std::time_t timeElapsed = currentTime - it->second.connectionTime;
    std::cout<< "time : "<<timeElapsed<<std::endl;
    if (timeElapsed > 300)
    {
        it->second.isLoggedIn = false;
        return false;
    }
    std::string formData;
    if (!extractFormData(clientBuffer, formData))
    {
        ServerErrors::handleErrors(server, clientFd, 400);
        return false;
    }
    std::map<std::string, std::string> formFields = ResolvePaths::parseURLEncoded(formData);
    std::string formFilePath;
    if (!getFormFilePath(server, formFields["form_file"], formFilePath))
    {
        std::cerr << "❌ Could not resolve form file path." << std::endl;
        ServerErrors::handleErrors(server, clientFd, 500);
        return false;
    }
    std::string username = findUsernameBySessionId(formFilePath, sessionId);
    std::cout << "username :" << username << std::endl;
    if (username.empty())
    {
        std::cerr << "❌ No active session found." << std::endl;
        return false;
    }
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Set-Cookie: session_id=" + sessionId + "; Path=/\r\n";
    response += "Content-Type: text/plain\r\n";
    response += "Content-Length: " + ServerUtils::numberToString(username.size()) + "\r\n";
    response += "Connection: close\r\n\r\n";
    response += username;
    send(clientFd, response.c_str(), response.size(), 0);
    shutdown(clientFd, SHUT_WR);
    close(clientFd);
    return true;
}



void ServerPost::handlePostRequest(Server &server, int clientFd, HttpRequestParser& parser, std::vector<char>& clientBuffer)
{
    std::string filePath = ResolvePaths::resolveFilePath(server, parser);
    std::string url = parser.getPath();
    if (ResolvePaths::findLocation(server, url) && !ResolvePaths::isMethodAllowed(server, filePath, "POST", url))
    {
        std::cerr << "❌ Error: POST not allowed for this location." << std::endl;
        ServerErrors::handleErrors(server, clientFd, 405);
        return;
    }
    std::map<std::string, std::string> headers = parser.getHeaders();
    std::cout << "🔍 Headers recibidos:" << std::endl;
    for (std::map<std::string, std::string>::iterator it = headers.begin(); it != headers.end(); ++it)
    {
        std::cout << it->first << ": " << it->second << std::endl;
    }
    std::map<std::string, std::string>::iterator itCtype = headers.find("Content-Type");
    std::map<std::string, std::string> cookies = parser.getCookies();
    std::map<std::string, std::string>::iterator itCookies = cookies.find("session_id");
    std::string contentType = "";
    if (itCtype == headers.end())
        contentType = "application/octet-stream";
    else
        contentType = itCtype->second;
    std::string sessionId = "";
    if (itCookies == cookies.end())
    {
	    std::cerr << "ℹ️ No Session ID found. Generating new session..." << std::endl;
	    sessionId = ServerUtils::generateSessionId();
	    SessionData newSession;
	    newSession.sessionId = sessionId;
	    newSession.isLoggedIn = false;
	    newSession.connectionTime = std::time(NULL);
	    server._clientSessions[clientFd] = newSession;
    }
    else 
	    sessionId = ServerUtils::trim(itCookies->second);
    std::map<std::string, std::string>::iterator itContentLength = headers.find("Content-Length");
    if (itContentLength != headers.end() && itContentLength->second == "0")
    {
        std::cout << "manda 204" << std::endl;
        ServerErrors::handleErrors(server, clientFd, 204);
        return;
    }
    if (parser.getPath() == "/check-session")
    {
        std::map<int, SessionData>::iterator it = server._clientSessions.find(clientFd);
        if (it == server._clientSessions.end() || !it->second.isLoggedIn)
            handleFormLogin(server, clientFd, clientBuffer);
        else if (!checkClientSession(server, clientFd, clientBuffer, sessionId))
            handleFormLogin(server, clientFd, clientBuffer);
    }
    else if (parser.getPath() == "/login")
        handleFormLogin(server, clientFd, clientBuffer);
    else if (parser.getPath() == "/submit-form" || contentType.find("application/x-www-form-urlencoded")!= std::string::npos)
        handleFormSubmission(server, clientFd, clientBuffer, sessionId, parser.getPath());
    else if (contentType.find("multipart/form-data") != std::string::npos)
        handleFileUpload(server, clientFd, clientBuffer);
    else if (contentType == "application/octet-stream")
        ServerGet::handleRawPost(clientFd);
    else
        ServerErrors::handleErrors(server, clientFd, 415);
}
