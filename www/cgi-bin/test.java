import java.io.BufferedReader;
import java.io.InputStreamReader;

public class test {
    public static void main(String[] args) throws Exception {
        System.out.println("Content-Type: text/html\n");
        
        String method = System.getenv("REQUEST_METHOD");
        
        if ("POST".equalsIgnoreCase(method)) {
            int contentLength = Integer.parseInt(System.getenv("CONTENT_LENGTH"));
            BufferedReader reader = new BufferedReader(new InputStreamReader(System.in));
            char[] postData = new char[contentLength];
            reader.read(postData, 0, contentLength);
            
            String name = "Guest";
            String postDataString = new String(postData);
            if (postDataString.startsWith("name=")) {
                name = postDataString.split("=")[1];
            }
            
            System.out.println("<h1>Welcome, " + name + "!</h1>");
        } else {
            System.out.println("<h1>Hello! This is a static message from JAVA SCRIPT.</h1>");
        }
    }
}

