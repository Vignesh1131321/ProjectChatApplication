package AuthServer;
import java.io.*;
import java.net.*;
import java.util.HashMap;

public class AuthServer {
    private static final int PORT = 12224;
    private static final String DATABASE_FILE = "database.txt";
    private HashMap<String, String> userDatabase = new HashMap<>();

    public static void main(String[] args) {
        new AuthServer().startServer();
    }

    public void startServer() {
        loadDatabase(); // Load user data from the file
        try (ServerSocket serverSocket = new ServerSocket(PORT)) {
            System.out.println("Server started on port " + PORT);
            while (true) {
                Socket clientSocket = serverSocket.accept();
                System.out.println("Client connected: " + clientSocket.getInetAddress());
                new Thread(() -> handleClient(clientSocket)).start();
            }
        } catch (IOException e) {
            System.err.println("Error starting server: " + e.getMessage());
        }
    }

    private void handleClient(Socket clientSocket) {        // Handle the client connection
        try (
            BufferedReader in = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));
            PrintWriter out = new PrintWriter(clientSocket.getOutputStream(), true)
        ) {
            String command;
            while ((command = in.readLine()) != null) { // Continuously read commands
                System.out.println("Received command: " + command);

                switch (command.toUpperCase()) {
                    case "LOGIN":
                        handleLogin(in, out);
                        break;
                    case "REGISTER":
                        handleRegister(in, out);
                        break;
                    default:
                        out.println("ERROR: Unknown command");
                        break;
                }
            }
        } catch (IOException e) {
            System.err.println("Error handling client: " + e.getMessage());
        } finally {
            try {
                clientSocket.close();
                System.out.println("Client disconnected.");
            } catch (IOException e) {
                System.err.println("Error closing client socket: " + e.getMessage());
            }
        }
    }

    private void handleLogin(BufferedReader in, PrintWriter out) throws IOException {       // Handle the login command
        String username = in.readLine();
        String password = in.readLine();

        System.out.println("Attempting login for user: " + username);

        if (userDatabase.containsKey(username) && userDatabase.get(username).equals(password)) {
            out.println("true"); // Login successful
            System.out.println("Login successful for user: " + username);
        } else {
            out.println("false"); // Login failed
            System.out.println("Login failed for user: " + username);
        }
    }

    private void handleRegister(BufferedReader in, PrintWriter out) throws IOException {        // Handle the register command
        String username = in.readLine();
        String password = in.readLine();

        System.out.println("Attempting to register user: " + username);

        if (userDatabase.containsKey(username)) {
            out.println("false"); // Registration failed
            System.out.println("Registration failed: User already exists.");
        } else {
            userDatabase.put(username, password);
            saveDatabase(); // Save to the database file
            out.println("true"); // Registration successful
            System.out.println("User registered successfully: " + username);
        }
    }

    private void loadDatabase() {       // Load the user database from the file
        try (BufferedReader reader = new BufferedReader(new FileReader(DATABASE_FILE))) {
            String line;
            while ((line = reader.readLine()) != null) {
                String[] parts = line.split(",");
                if (parts.length == 2) {
                    userDatabase.put(parts[0], parts[1]);
                }
            }
            System.out.println("Database loaded successfully.");
        } catch (FileNotFoundException e) {
            System.out.println("Database file not found. Starting with an empty database.");
        } catch (IOException e) {
            System.err.println("Error loading database: " + e.getMessage());
        }
    }

    private void saveDatabase() {       // Save the user database to the file
        try (PrintWriter writer = new PrintWriter(new FileWriter(DATABASE_FILE))) {
            for (var entry : userDatabase.entrySet()) {
                writer.println(entry.getKey() + "," + entry.getValue());
            }
            System.out.println("Database saved successfully.");
        } catch (IOException e) {
            System.err.println("Error saving database: " + e.getMessage());
        }
    }
}
