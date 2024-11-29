import AuthUser.AuthUser;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.HashSet;

public class ChatSession {
    private HashSet<String> activeUsers = new HashSet<>();

    public boolean login(String username, String password, AuthUser authUser,BufferedReader console, PrintWriter out, BufferedReader in) throws IOException {
        if (authUser.handleLogin(username, password,console,out,in)) { // Call the handleLogin method from AuthUser
            activeUsers.add(username);
            return true;
        }
        return false;
    }

    public void logout(String username) {       // Remove the user from the activeUsers set
        activeUsers.remove(username);
    }

    public boolean isAuthenticated(String username) {   // Check if the user is in the activeUsers set
        return activeUsers.contains(username);
    }
}
