import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;
import java.util.Scanner;

public class ChatClient {   // Client class to run the chat application
    private ChatApplication chatApp;
    private static final String SERVER_ADDRESS = "192.168.43.158"; // Replace with server's IP address
    private static final int PORT = 12224;

    public ChatClient() {
        chatApp = new ChatApplication(); // Initialize your ChatApplication class
        
    }

    public void run(BufferedReader console, PrintWriter out, BufferedReader in) {
        Scanner scanner = new Scanner(System.in);

        String serverAddress = "192.168.43.158";
        int port = 12223;
      
        String username="";
        String phoneNumber;
        while (true) {
            System.out.println("Welcome to the Chat Application!");
            System.out.println("1. Register");
            System.out.println("2. Login");
            System.out.println("3. Exit");

            System.out.print("Choose an option: ");
            int choice = scanner.nextInt();
            scanner.nextLine(); // Consume newline

            if (choice == 1) {
                // Register new user
                System.out.print("Enter username for registration: ");
                username = scanner.nextLine();
                System.out.print("Enter phone number for registration: ");
                phoneNumber = scanner.nextLine();
                System.out.print("Enter password for registration: ");
                String password = scanner.nextLine();
                if (chatApp.register(username, password,console,out,in)) {
                    System.out.println("Registration successful. You can now log in.");
                } else {
                    System.out.println("Registration failed. Phone number may already exist.");
                }

            }
            else if (choice == 2) {
                // Login existing user
                System.out.print("Enter username: ");
                username = scanner.nextLine();
                System.out.print("Enter password: ");
                String password = scanner.nextLine();
                if (chatApp.authenticate(username, password,console,out,in)) {
                    System.out.println("Login successful!");
                    try {
                        chatApp.connectToServer(username, serverAddress, port);           
                    } catch (Exception e) {
                        System.err.println("Error connecting to the server: " + e.getMessage());
                    }
                } 
                else {
                    System.out.println("Login failed. Check phone number and password.");
                }

            } 
            else if (choice == 3) {
                // Exit the application
                System.out.println("Exiting the application.");
                break;
            } 
            else {
                System.out.println("Invalid choice. Please try again.");
            }
        }
        
        scanner.close();
    }


    public static void main(String[] args) {
        ChatClient client = new ChatClient();
        
        try(
        Socket socket = new Socket(SERVER_ADDRESS, PORT);
        PrintWriter out = new PrintWriter(socket.getOutputStream(), true);
        BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
        BufferedReader console = new BufferedReader(new InputStreamReader(System.in));){

            client.run(console,out,in);  // Run the client application
        }
        catch(IOException e){
            e.printStackTrace();
        }
    }
}
