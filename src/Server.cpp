#include <iostream>
#include <unordered_map>
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

struct ExpirableValue {
    std::string value; 
    std::chrono::time_point<std::chrono::system_clock> expiryTime;
    bool hasExpiry; 
};


void handleClient(int client_fd){
  std::unordered_map<std::string, ExpirableValue> dict;
  std::string ok = "+OK\r\n";
  std::string pong = "+PONG\r\n";
  std::string failure = "$-1\r\n";

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
    std::cout << "Received from client: " << str << std::endl;
    std::vector<std::string> tokens = splitRedisCommand(str, "\r\n", 2);
    for (const std::string& token: tokens) {
        std::cout << "***" << token << "***" << std::endl;
    }
    // Construct Lowercase command
    std::string lCommand = "";
    for(auto c : tokens[2]) {
      lCommand += tolower(c);
    }

    // std::cout << "*" << lCommand << "*" << std::endl;
      if(lCommand == "ping") {
        std::cout << "+" << pong << "+" << std::endl;
        send(client_fd, pong.data(), pong.length(), 0);

      } else if(lCommand == "echo") {
        // $3\r\nhey\r\n
        std::string echoRes = tokens[3] + "\r\n"  + tokens[4] + "\r\n";
        std::cout << "+" << echoRes << "+" << std::endl;
        send(client_fd, echoRes.data(), echoRes.length(), 0);

      } else if (lCommand == "set") {
        std::string value = tokens[5] + "\r\n" + tokens[6] + "\r\n";
        std::cout << value  << std::endl;
        bool hasExpiry = false;
        std::chrono::time_point<std::chrono::system_clock> expiryTime = std::chrono::system_clock::now();
        

        //if additional flags
        if (tokens.size() > 8) {

          //convert flag to lowercase
          std::string flag = "";
          for(auto c : tokens[8]) {
            flag += tolower(c);
          }
          std::cout <<"this is the flag encountered" << flag << std::endl;
          std::cout <<"this is at pos 9" << tokens[9] << std::endl;
          std::cout <<"this is at pos 10" << tokens[10] << std::endl;

          if (flag == "px"){
            hasExpiry = true;
            std::cout << "entered px flag with time" << tokens[3]  << std::endl;
            int expiryDelay = std::stoi(tokens[3]);

            expiryTime = std::chrono::system_clock::now() + std::chrono::milliseconds(expiryDelay);
            
          }
        }

        //add to dict with expiry
        dict[tokens[4]] = {value, expiryTime, hasExpiry};
        send(client_fd, ok.data(), ok.length(), 0);

      } else if (lCommand == "get") {
        if (dict.count(tokens[4]) == 0) {
          send(client_fd, failure.data(), failure.length(), 0);
        } else {
          ExpirableValue res = dict[tokens[4]];
          if ((res.hasExpiry) && (std::chrono::system_clock::now() > res.expiryTime)) {
            std::cout << "expiry and failure"  << std::endl;
            send(client_fd, failure.data(), failure.length(), 0);
          } else {
            send(client_fd, res.value.data(), res.value.length(), 0);
          }          
        }
      }  

    // std::cout << "Received from client: " << str << std::endl;
    // send(client_fd, response, strlen(response), 0 );
  }
  close(client_fd);
}

int main(int argc, char **argv) {
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket\n";
   return 1;
  }
  
  // set SO_REUSEADDR to ensure that we don't run into 'Address already in use' errors when server restarts
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }
  
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(6379);
 
  
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
