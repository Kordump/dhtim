#include <thread>
#include <chrono>
#include <iostream>
#include <unordered_map>

#include <readline/readline.h>
#include <readline/history.h>

#include <opendht.h>

void disp(std::string content)
{
    std::cout << "\r\n"
    "\x1b[A\x1b[A " << content << "\n"
    "\x1b[2K" << std::endl;
}

std::chrono::milliseconds get_timestamp()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch());
}

std::string input(const std::string& prompt)
{
    const char* line = readline(prompt.c_str());
    if(line && *line)
        add_history(line);

    if(line)
        return std::string(line);
    return std::string("");
}

struct msg : public std::string
{
    msg(std::string text)
        : std::string(
            "[" + std::to_string(get_timestamp().count()) + "]"
            " <" + username + "> " + text)
    { };

    operator dht::Value()
    {
        return dht::Value::pack(std::string(*this));
    }

    static std::string username;
};

std::string msg::username = "Anonymous";

using map_type = std::unordered_map<std::string, std::chrono::milliseconds>;
using pair_type = map_type::value_type;

int main(int argc, char** argv)
{
    // Create an unordered_map
    map_type map;

    // Use a opendht node.
    dht::DhtRunner node;

    // Create a brand new identity.
    auto node_identity = dht::crypto::generateIdentity();

    // Run a node with an OS-selected port, using the crypto-layer of opendht.
    node.run(0, node_identity, true);

    // Join the network through an already known node.
    node.bootstrap(argv[1], argv[2]);

    // User input
    msg::username = input("Nom d'utilisateur : ");
    std::string chan = input("Salon à joindre : ");

    // Pack&Put data on the DHT
    node.putSigned(chan, msg("Enter the chatroom."));

    // Retrieve past, old unordered messages.
    node.get(chan,
        [&](const std::vector<std::shared_ptr<dht::Value>>& values) {

            // Callback used for every value found.
            for (const auto& value : values)
            {
                std::string content = value->unpack<std::string>();
                map.insert(std::make_pair(content, get_timestamp()));
            }

            return true;
        });

    // Watch for incoming messages
    node.listen(chan,
        [&](const std::vector<std::shared_ptr<dht::Value>>& values) {

            // Callback used for every value found.
            for (const auto& value : values)
            {
                std::string content = value->unpack<std::string>();
                if(!map.count(content))
                {
                    auto stamp = get_timestamp();
                    map.insert(std::make_pair(content, stamp));
                    disp(content);
                }
            }

            // Continue or else stop the search ?
            return true;
        });

    // User input
    for(;;)
    {
        std::string output = input("");
        std::cout << "\x1b[A\x1b[2K\r";

        if(!output.empty())
        {
            // Insert the message in the DHT.
            auto content = msg(output);
            node.putSigned(chan, content);

            // Register-it locally and instant display.
            auto stamp = get_timestamp();
            map.insert(std::make_pair(msg(content), stamp));
            disp(content);
        }
    }

    // Wait threads
    node.join();
    return 0;
}

