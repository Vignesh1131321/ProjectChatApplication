import AuthUser.AuthUser;
import java.io.*;
import java.util.Scanner;

public class ChatApplication {
    private Scanner scanner;
    private AuthUser authUser = new AuthUser();
    private ChatSession session = new ChatSession();


    public ChatApplication() {
        scanner = new Scanner(System.in);
    }
    // Register a new user
    public boolean register(String username,  String password, BufferedReader console, PrintWriter out, BufferedReader in){
        try{
            if(authUser.handleRegister(username , password ,console,out,in)){
                return true;
            }
            else{
                return false;
            }
        }
        catch(IOException e){
            e.printStackTrace();
            return false;
        }
    }

    // Existing authentication logic
    public boolean authenticate(String username, String password, BufferedReader console, PrintWriter out, BufferedReader in){
        try{
            if(session.login(username, password, authUser, console, out, in)){
                return true;
            }
            else{
                return false;
            }
        }
        catch(IOException e){
            e.printStackTrace();
            return false;
        }
    }
    
    static {
        System.loadLibrary("ChatServerConnector");
        System.loadLibrary("ChatClient");
    }

    public native void startServer(int port);       // Start the server
    public native void connectToServer(String username, String serverAddress, int port);        // Connect to the server

    public static void main(String[] args) {

        ChatApplication app = new ChatApplication();
        app.startServer(12223);
    }
}
