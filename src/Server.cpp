#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <thread>
#include <chrono>
#include <sys/select.h>
#include <vector>
#include <pthread.h>

std::vector<std::string> splitRedisCommand(std::string input, std::string separator, int separatorLength ) {
  std::size_t foundSeparator = input.find(separator);
  std::vector<std::string> result;
  if (foundSeparator == std::string::npos) {
      result.push_back(input);
  }
  while (foundSeparator != std::string::npos) {
      std::string splitOccurrence = input.substr(0, foundSeparator);
      result.push_back(splitOccurrence);
      input = input.substr(foundSeparator + separatorLength, input.length()-foundSeparator+separatorLength);
      foundSeparator = input.find(separator);
  }
  return result;
}


void handleClient(int client_fd){
  const char *response ="+PONG\r\n";

  while (true) {
    char buffer[1024];
    int bytes_received = recv(client_fd, buffer, 1024, 0);
    if (bytes_received <= 0) {
      std::cout << "Client disconnected\n";
      break;
    }
    //test
    buffer[bytes_received] = '\0';
    std::string str(buffer, strlen(buffer)); 
    std::vector<std::string> tokens = splitRedisCommand(str, "\r\n", 2);
    for (const std::string& token: tokens) {
        std::cout << "***" << token << "***" << std::endl;
    }
    // Lowercase command
    std::string lCommand = "";
    for(auto c : tokens[2]) {
        lCommand += tolower(c);
    }

    std::cout << "*" << lCommand << "*" << std::endl;
      if(lCommand == "ping") {
         std::string pongRes = "+PONG\r\n";
         std::cout << "+" << pongRes << "+" << std::endl;
         send(clientSocketId, pongRes.data(), pongRes.length(), 0);
      } else if(lCommand == "echo") {
         // $3\r\nhey\r\n
         std::string echoRes = tokens[3] + "\r\n"  + tokens[4] + "\r\n";
         std::cout << "+" << echoRes << "+" << std::endl;
1
         send(clientSocketId, echoRes.data(), echoRes.length(), 0);
      }
   

    // std::cout << "Received from client: " << str << std::endl;
    // send(client_fd, response, strlen(response), 0 );
    
  
  }
  close(client_fd);
  
}



int main(int argc, char **argv) {
  // You can use print statements as follows for debugging, they'll be visible when running tests.
  std::cout << "Logs from your program will appear here!\n";

  // Uncomment this block to pass the first stage
  //
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket\n";
   return 1;
  }
  
  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }
  
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(6379);
  //bool keep_running = true;
  //const int TIMEOUT_SECONDS = 10; 
 
  
  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 6379\n";
    return 1;
  }
  
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }
  
  std::vector<std::thread> threads;
  std::cout << "Waiting for a client to connect...\n";
  
  while (true){
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
    std::cout << "Client connected\n";
    threads.emplace_back(handleClient, client_fd);
  }
  
  
  
  close(server_fd);

  return 0;
}
