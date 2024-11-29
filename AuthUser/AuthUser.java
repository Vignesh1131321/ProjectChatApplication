package AuthUser;
import java.io.*;

public class AuthUser {

    public boolean handleLogin(String username, String password,BufferedReader console, PrintWriter out, BufferedReader in) throws IOException {
        out.println("LOGIN");           // Send the server the LOGIN command

        out.println(username);
        out.println(password);

        String response = in.readLine();
        if (Boolean.parseBoolean(response)) {
            return true;
        } else {
            return false;
        }
    }

    public boolean handleRegister(String username,String password,BufferedReader console, PrintWriter out, BufferedReader in) throws IOException{
        out.println("REGISTER");        // Send the server the REGISTER command
        
        out.println(username);
        out.println(password);

        String response = in.readLine();
        
        if (Boolean.parseBoolean(response)) {
            return true;
        } else {
            return false;
        }
    }
}
