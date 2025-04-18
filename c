

// first microservice ====================================================================

here when we hit first microservices controller then this controller is calling the webclient function and then  webcllient hits third
party server like mockapi to fetch json data so this response is going to first microservices now we are defining kafka topic and producer
in the first microservices and whatever response coming form mockapi (using webclient) we are sending that json response data to second
microservices where in second microservices we are using kafka consumer and that kafka consumer is also targeting the kafka topic of first
microservices (other microservices 2 using aapache kafka) as a event microservices as a distribution system now we can save that 
json data which is consumed by second json data 


note when we send json data over kafka from one microservices to other then in first microservices we have to convert java object to 
json string format and while receiving this incoming json string format in kafka consumer at second microservices we again convert 
json format to java object so that we can use it in second microsrvices and save in dbor can use setter and getter on that converted java 
object and note that this conversion is possible because of jackson object mapper 

=================================  FIRST MICROSERVICES (PRODUCER) ========================================================

// basic webclient 

// model ==============================================================================================================================================================

package com.example.basic_webclient.model;

import lombok.*;

public class User {
    private Long id;
    private String name;
    private String city;

    public User(){}


    public User(Long id, String name, String city) {
        this.id = id;
        this.name = name;
        this.city = city;
    }

    public Long getId() {
        return id;
    }

    public void setId(Long id) {
        this.id = id;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public String getCity() {
        return city;
    }

    public void setCity(String city) {
        this.city = city;
    }

    @Override
    public String toString() {
        return "User{" +
                "id=" + id +
                ", name='" + name + '\'' +
                ", city='" + city + '\'' +
                '}';
    }
}



// service ================================================================================================

package com.example.basic_webclient.service;

import com.example.basic_webclient.model.User;
import com.fasterxml.jackson.core.JsonProcessingException;
import org.springframework.stereotype.Service;

import java.util.List;


@Service
public interface UserService {
    User getUserById(String userId) throws JsonProcessingException;

    List<User> getUsers();

    User createUser(User user);

    List<User> createListOfUser(List<User>users);

    void deleteUserById(Long userId);

    User updateUser(Long id, User updatedUser);

}


// service impl =============================================================================================================

package com.example.basic_webclient.service.impl;

import com.example.basic_webclient.client.UserClient;
import com.example.basic_webclient.model.User;
import com.example.basic_webclient.service.UserService;
import com.fasterxml.jackson.core.JsonProcessingException;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

import java.util.List;

@Service
public class UserServiceImpl implements UserService {

    @Autowired
    private UserClient userClient;


    @Override
    public User getUserById(String userId) throws JsonProcessingException {
        return userClient.getPost(userId);
    }

    @Override
    public List<User> getUsers() {
        return userClient.getPostList();
    }

    @Override
    public User createUser(User user) {
        return userClient.createUser(user);
    }

    @Override
    public List<User> createListOfUser(List<User> users) {
        return userClient.createListOfUser(users);
    }

    @Override
    public void deleteUserById(Long userId) {
        userClient.deleteUserById(userId);
    }

    @Override
    public User updateUser(Long userId, User updatedUser) {
        return userClient.updateUser(userId,updatedUser);
    }
}



// client ===================================================================================================================

package com.example.basic_webclient.client;

import com.example.basic_webclient.model.User;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.kafka.core.KafkaTemplate;
import org.springframework.stereotype.Component;
import org.springframework.web.reactive.function.client.WebClient;

import java.util.List;

@Component
public class UserClient {

    @Autowired
    @Qualifier("jsonPlaceholderWebClient")
    private WebClient webClient;

    @Autowired
    private KafkaTemplate<Object, String> kafkaTemplate;


    @Autowired
    private ObjectMapper objectMapper;


    // ✅ Method to send message to Kafka topic
    public void sendToTopic1(User user) throws JsonProcessingException {
        String json = objectMapper.writeValueAsString(user);
        kafkaTemplate.send("firstTopic15", json);
    }


    public User getPost(String userId) throws JsonProcessingException {
        User user = webClient.get()
                .uri("/crud/" + userId)
                .retrieve()
                .bodyToMono(User.class)
                .block(); // Note: blocking in reactive is not ideal, but OK for simple use cases

        // ✅ Send user info (as string) to Kafka topic
        if (user != null) {
            sendToTopic1(user); // or convert to JSON using ObjectMapper
        }

        return user;
    }

    public List<User> getPostList() {
        return webClient.get()
                .uri("/crud")
                .retrieve()
                .bodyToFlux(User.class)
                .collectList()
                .block(); // For simplicity; in production, use reactive chains
    }


    public User createUser(User user) {
        return webClient.post()
                .uri("/crud")
                .bodyValue(user) // send the user object in the request body
                .retrieve()
                .bodyToMono(User.class)
                .block(); // synchronous; okay if you're not in a reactive flow
    }


    public List<User> createListOfUser(List<User> users) {
        return webClient.post()
                .uri("/crud") // use your correct endpoint
                .bodyValue(users)  // 👈 send list directly
                .retrieve()
                .bodyToFlux(User.class)
                .collectList()
                .block();
    }


    public void deleteUserById(Long userId) {
        webClient.delete()
                .uri("/crud/" + userId)
                .retrieve()
                .bodyToMono(Void.class)
                .block(); // Again, blocking is not ideal in reactive apps
    }


    public User updateUser(Long userId, User updatedUser) {
        return webClient.put()
                .uri("/crud/" + userId)
                .bodyValue(updatedUser) // Sends the updated user as the request body
                .retrieve()
                .bodyToMono(User.class)
                .block(); // Blocking for simplicity
    }

}


// webclient config ========================================================================================================

package com.example.basic_webclient.config;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.web.reactive.function.client.WebClient;

@Configuration
public class WebClientConfig {

    @Bean(name = "jsonPlaceholderWebClient")
    public WebClient userServiceWebClient(WebClient.Builder builder) {
        return builder
                .baseUrl("https://67c35ac91851890165aedae8.mockapi.io")
                .build();
    }

}


// controller ===============================================================================================================

package com.example.basic_webclient.controller;

import com.example.basic_webclient.model.User;
import com.example.basic_webclient.service.UserService;
import com.fasterxml.jackson.core.JsonProcessingException;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.ResponseEntity;
import org.springframework.kafka.core.KafkaTemplate;
import org.springframework.web.bind.annotation.*;

import java.util.List;

@RestController
@RequestMapping("/api")
@CrossOrigin

public class UserController {

    @Autowired
    private UserService userService;



    @GetMapping("/get/{id}")
    public ResponseEntity<?> fetchPostById(@PathVariable String id) throws JsonProcessingException {
        User user=userService.getUserById(id);
        return user!=null ? ResponseEntity.ok(user) : ResponseEntity.notFound().build();
    }


    @GetMapping("/getAll")
    public ResponseEntity<?> fetchAll(){
        List<User> user=userService.getUsers();
        return !user.isEmpty() ? ResponseEntity.ok(user) : ResponseEntity.notFound().build();
    }


    @PostMapping("/create")
    public ResponseEntity<?> createUser(@RequestBody User user){
        User createdUser=userService.createUser(user);
        return createdUser!=null ? ResponseEntity.ok(user) : ResponseEntity.badRequest().build();
    }


    @PostMapping("/createAll")
    public ResponseEntity<?> createListOfUser(@RequestBody List<User> users){
        List<User> createdUserList=userService.createListOfUser(users);
        return !createdUserList.isEmpty() ? ResponseEntity.ok(createdUserList) : ResponseEntity.badRequest().build();
    }


    @DeleteMapping("/deleteById/{id}")
    public ResponseEntity<?> createListOfUser(@PathVariable Long id){
        try{
            userService.deleteUserById(id);
            return ResponseEntity.noContent().build();
        }
        catch(Exception e){
            return ResponseEntity.notFound().build();
        }
    }

    @PutMapping("/update/{id}")
    public ResponseEntity<?> updateUser(@PathVariable Long id, @RequestBody User updatedUser){
        User user=userService.updateUser(id,updatedUser);
        return user!=null ? ResponseEntity.ok(user) : ResponseEntity.notFound().build();
    }

}


// topic ====================================================================================================================

package com.example.basic_webclient.topic;

import org.apache.kafka.clients.admin.NewTopic;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.kafka.config.TopicBuilder;

@Configuration
public class KafkaTopic {

    @Bean
    NewTopic myTopic1(){
        return TopicBuilder.name("firstTopic15").partitions(4).replicas(1).build();
    }

}



// APPLICATION PROPERTIES

spring.application.name=basic-webclient

# Kafka Bootstrap Server
spring.kafka.bootstrap-servers=localhost:9092

 spring.kafka.consumer.value-deserializer=org.springframework.kafka.support.serializer.JsonDeserializer

# ===============================
# Producer Properties
# ===============================
spring.kafka.producer.key-serializer=org.apache.kafka.common.serialization.StringSerializer
spring.kafka.producer.value-serializer=org.springframework.kafka.support.serializer.JsonSerializer



// TOTAL APPLICATION PROPERTIES (EXTRA IF NEEDED) =====================================================================

spring.application.name=basic-webclient

# Kafka Bootstrap Server
spring.kafka.bootstrap-servers=localhost:9092

## Kafka Consumer Configuration
#spring.kafka.consumer.auto-offset-reset=earliest
#spring.kafka.consumer.key-deserializer=org.apache.kafka.common.serialization.StringDeserializer
##spring.kafka.consumer.value-deserializer=org.apache.kafka.common.serialization.StringDeserializer

# If you're using JSON (optional):
 spring.kafka.consumer.value-deserializer=org.springframework.kafka.support.serializer.JsonDeserializer
# spring.kafka.consumer.properties.spring.json.trusted.packages=*
#
## Kafka Producer Configuration
#spring.kafka.producer.key-serializer=org.apache.kafka.common.serialization.StringSerializer
##spring.kafka.producer.value-serializer=org.apache.kafka.common.serialization.StringSerializer
#
## If you're sending Java objects as JSON (optional):
# spring.kafka.producer.value-serializer=org.springframework.kafka.support.serializer.JsonSerializer




# ===============================
# Producer Properties
# ===============================
spring.kafka.producer.key-serializer=org.apache.kafka.common.serialization.StringSerializer
spring.kafka.producer.value-serializer=org.springframework.kafka.support.serializer.JsonSerializer

#spring.kafka.producer.properties.partitioner.class=org.apache.kafka.clients.producer.RoundRobinPartitioner




// other microservices ============================================================================================




package com.example.SpringKafka.consumer;

import com.example.SpringKafka.model.User;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.kafka.annotation.KafkaListener;
import org.springframework.messaging.handler.annotation.Payload;
import org.springframework.stereotype.Service;

@Service
public class KafkaConsumer {

    @Autowired
    private ObjectMapper objectMapper;

    @KafkaListener(topics = "firstTopic15", groupId = "group1")
    public void sendToTopic1(String rawJson) throws JsonProcessingException {
        System.out.println("Raw message: " + rawJson);
            User user = objectMapper.readValue(rawJson, User.class);
            // Use getters or log the object
            System.out.println("Parsed User: " + user);
            System.out.println("User Name: " + user.getName());
            System.out.println("User City: " + user.getCity());
    }


}



// APPLICATION PROPERTIES



# ========== Kafka Common ==========
spring.kafka.bootstrap-servers=localhost:9092

# ========== Consumer ==========
spring.kafka.consumer.group-id=group1
spring.kafka.consumer.auto-offset-reset=earliest
spring.kafka.consumer.key-deserializer=org.apache.kafka.common.serialization.StringDeserializer
spring.kafka.consumer.value-deserializer=org.springframework.kafka.support.serializer.ErrorHandlingDeserializer

# Delegate to JsonDeserializer
spring.kafka.consumer.properties.spring.deserializer.value.delegate.class=org.springframework.kafka.support.serializer.JsonDeserializer

# Allow JSON deserialization from any package (or just your model package)
spring.kafka.consumer.properties.spring.json.trusted.packages=*

# Always treat incoming JSON as User class
spring.kafka.consumer.properties.spring.json.value.default.type=com.example.SpringKafka.model.User

# ========== Producer ==========
spring.kafka.producer.key-serializer=org.apache.kafka.common.serialization.StringSerializer
spring.kafka.producer.value-serializer=org.springframework.kafka.support.serializer.JsonSerializer

# Optional: don't include type headers from producer (keeps payload clean)
spring.kafka.producer.properties.spring.json.add.type.headers=false

# Server
server.port=9008



// TOTAL APPLICATION PROPERTIES (EXTRA IF NEEDED) =====================================================================

###spring.application.name=SpringKafka
###
#### Kafka Bootstrap Server
###spring.kafka.bootstrap-servers=localhost:9092
###
#### Kafka Consumer Configuration
###spring.kafka.consumer.auto-offset-reset=earliest
###spring.kafka.consumer.key-deserializer=org.apache.kafka.common.serialization.StringDeserializer
####spring.kafka.consumer.value-deserializer=org.apache.kafka.common.serialization.StringDeserializer
###
#### If you're using JSON (optional):
### spring.kafka.consumer.value-deserializer=org.springframework.kafka.support.serializer.JsonDeserializer
#### spring.kafka.consumer.properties.spring.json.trusted.packages=*
###
#### Kafka Producer Configuration
###spring.kafka.producer.key-serializer=org.apache.kafka.common.serialization.StringSerializer
####spring.kafka.producer.value-serializer=org.apache.kafka.common.serialization.StringSerializer
###
#### If you're sending Java objects as JSON (optional):
### spring.kafka.producer.value-serializer=org.springframework.kafka.support.serializer.JsonSerializer
###
####spring.kafka.producer.properties.partitioner.class=org.apache.kafka.clients.producer.RoundRobinPartitioner
###
###server.port=9008
##
##
###
#### ===============================
#### Kafka Common Configuration
#### ===============================
###spring.kafka.bootstrap-servers=localhost:9092
###spring.kafka.consumer.group-id=group1
###
#### ===============================
#### Consumer Properties
#### ===============================
###spring.kafka.consumer.key-deserializer=org.apache.kafka.common.serialization.StringDeserializer
###spring.kafka.consumer.value-deserializer=org.springframework.kafka.support.serializer.JsonDeserializer
###
#### Important: Trust your package to deserialize User object
###spring.kafka.consumer.properties.spring.json.trusted.packages=com.example.SpringKafka.model
###
#### ===============================
#### Producer Properties
#### ===============================
###spring.kafka.producer.key-serializer=org.apache.kafka.common.serialization.StringSerializer
###spring.kafka.producer.value-serializer=org.springframework.kafka.support.serializer.JsonSerializer
###server.port=9008
##
##
##
##
##
### ===============================
### Kafka Common Configuration
### ===============================
##spring.kafka.bootstrap-servers=localhost:9092
###spring.kafka.consumer.group-id=group1
##
### ===============================
### Consumer Properties
### ===============================
##spring.kafka.consumer.key-deserializer=org.apache.kafka.common.serialization.StringDeserializer
###spring.kafka.consumer.value-deserializer=org.springframework.kafka.support.serializer.JsonDeserializer
##
### Important: Trust your package to deserialize User object
###spring.kafka.consumer.properties.spring.json.trusted.packages=com.example.SpringKafka.model
##
### ===============================
### Producer Properties
### ===============================
##spring.kafka.producer.key-serializer=org.apache.kafka.common.serialization.StringSerializer
##spring.kafka.producer.value-serializer=org.springframework.kafka.support.serializer.JsonSerializer
##server.port=9008
##
###spring.kafka.consumer.properties.spring.json.trusted.packages=*
##
### Use ErrorHandlingDeserializer to avoid app crash on bad messages
###spring.kafka.consumer.value-deserializer=org.springframework.kafka.support.serializer.ErrorHandlingDeserializer
##spring.kafka.consumer.properties.spring.deserializer.value.delegate.class=org.springframework.kafka.support.serializer.JsonDeserializer
##
### ? Trust all packages (or restrict to yours)
##spring.kafka.consumer.properties.spring.json.trusted.packages=*
##
### ? Always deserialize as this class, no matter the producer's type header
##spring.kafka.consumer.properties.spring.json.value.default.type=com.example.SpringKafka.model.User
##spring.kafka.consumer.value-deserializer=org.springframework.kafka.support.serializer.JsonDeserializer
#
#
#
## ===============================
## Application Info
## ===============================
#spring.application.name=SpringKafka
#server.port=9008
#
## ===============================
## Kafka Common Configuration
## ===============================
#spring.kafka.bootstrap-servers=localhost:9092
#
## ===============================
## Kafka Consumer Configuration
## ===============================
#spring.kafka.consumer.group-id=group1
#spring.kafka.consumer.auto-offset-reset=earliest
#
## Use ErrorHandlingDeserializer to prevent crashes on bad messages
#spring.kafka.consumer.key-deserializer=org.apache.kafka.common.serialization.StringDeserializer
#spring.kafka.consumer.value-deserializer=org.springframework.kafka.support.serializer.ErrorHandlingDeserializer
#
## Delegate actual deserialization to JsonDeserializer
#spring.kafka.consumer.properties.spring.deserializer.value.delegate.class=org.springframework.kafka.support.serializer.JsonDeserializer
#
## Allow deserialization from specific package or all
#spring.kafka.consumer.properties.spring.json.trusted.packages=*
#
## Optional: Force the expected Java type (no need for __TypeId__ header)
#spring.kafka.consumer.properties.spring.json.value.default.type=com.example.SpringKafka.model.User
#
## ===============================
## Kafka Producer Configuration
## ===============================
#spring.kafka.producer.key-serializer=org.apache.kafka.common.serialization.StringSerializer
#spring.kafka.producer.value-serializer=org.springframework.kafka.support.serializer.JsonSerializer


# ========== Kafka Common ==========
spring.kafka.bootstrap-servers=localhost:9092

# ========== Consumer ==========
spring.kafka.consumer.group-id=group1
spring.kafka.consumer.auto-offset-reset=earliest
spring.kafka.consumer.key-deserializer=org.apache.kafka.common.serialization.StringDeserializer
spring.kafka.consumer.value-deserializer=org.springframework.kafka.support.serializer.ErrorHandlingDeserializer

# Delegate to JsonDeserializer
spring.kafka.consumer.properties.spring.deserializer.value.delegate.class=org.springframework.kafka.support.serializer.JsonDeserializer

# Allow JSON deserialization from any package (or just your model package)
spring.kafka.consumer.properties.spring.json.trusted.packages=*

# Always treat incoming JSON as User class
spring.kafka.consumer.properties.spring.json.value.default.type=com.example.SpringKafka.model.User

# ========== Producer ==========
spring.kafka.producer.key-serializer=org.apache.kafka.common.serialization.StringSerializer
spring.kafka.producer.value-serializer=org.springframework.kafka.support.serializer.JsonSerializer

# Optional: don't include type headers from producer (keeps payload clean)
spring.kafka.producer.properties.spring.json.add.type.headers=false

# Server
server.port=9008



