#include <stdio.h>
#include <curl/curl.h>
#include <string>
#include <tgbot/tgbot.h>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <iostream>

using namespace std;

size_t WriteCallBack(void* contents, size_t size, size_t nmemb, void* userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Запрос HTTP через cURL
string httpReq(const string& url) {
    CURL* curl;
    CURLcode res;
    string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallBack);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    return readBuffer; // Возврат ответа
}

int main() {

    setlocale(LC_ALL, "Russian");

    // Запуск бота
    TgBot::Bot bot("TOKEN");
    unordered_map<int64_t, string> userState;

    // Реакция на команду /start
    bot.getEvents().onCommand("start", [&bot, &userState](TgBot::Message::Ptr message) {
        bot.getApi().sendMessage(message->chat->id, "Enter City name, which temp i need to parse from API! (temp in K).");
        userState[message->chat->id] = "waiting_for_city"; // Установка состояния
        });

    // Обработка сообщений от пользователя
    bot.getEvents().onAnyMessage([&bot, &userState](TgBot::Message::Ptr message) {
        if (message->text == "/start") {
            return;
        }

        if (userState[message->chat->id] == "waiting_for_city") {
            std::string cityName = message->text;

            cout << "City Name: " << cityName << endl; // Логирование имени города

            // Запрос к API
            string apiUrl = "http://api.openweathermap.org/data/2.5/weather?q=" + cityName + "&appid=a0d8544bab3e33fba88a81b5ceb4a7f6";
            string weatherData = httpReq(apiUrl);

            if (weatherData.empty()) {
                bot.getApi().sendMessage(message->chat->id, "API Error.");
                return; // Завершение обработки сообщения, если ответ пустой
            }

            cout << "Weather Data: " << weatherData << endl; // Логирование данных о погоде

            try {
                auto jsonResp = nlohmann::json::parse(weatherData);
                if (jsonResp.contains("main") && jsonResp["main"].contains("temp")) {
                    double t = jsonResp["main"]["temp"].get<double>();
                    string temperatureMessage = "Temp in town " + cityName + ": " + to_string(t) + " K";

                    if (isValidUTF8(temperatureMessage)) {
                        bot.getApi().sendMessage(message->chat->id, temperatureMessage);
                    }
                    else {
                        bot.getApi().sendMessage(message->chat->id, "Error, invalid symbols.");
                    }
                }
                else {
                    bot.getApi().sendMessage(message->chat->id, "Cannot parse temp from " + cityName);
                }
            }
            catch (const nlohmann::json::parse_error& e) {
                bot.getApi().sendMessage(message->chat->id, "Parse error.");
            }

            userState[message->chat->id] = ""; // Сброс состояния
        }
             else {
                bot.getApi().sendMessage(message->chat->id, "Now Enter another city name");
                userState[message->chat->id] = "waiting_for_city";
             }
        });
            
          
   

    // Запуск бота с использованием TgLongPoll
    try {
        std::cout << "Bot is running..." << std::endl;
        TgBot::TgLongPoll longPoll(bot);
        while (true) {
            longPoll.start();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
