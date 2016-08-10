#include <thread>
#include <chrono>
#include <iostream>
#include <unordered_map>

#include <readline/readline.h>
#include <readline/history.h>

#include <opendht.h>

constexpr char const * default_bootstrap_host = "bootstrap.ring.cx";
constexpr char const * default_bootstrap_port = "4222";

// Nice display with VT100 terminal control escape sequences.
void disp(std::string content);

// Get timestamp in milliseconds.
std::chrono::milliseconds get_timestamp();

// Use GNU readline.
std::string input(const std::string& prompt, bool& eof);

// Simple format for our messages.
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
std::string msg::username;

// Map type used for message deduplication.
using map_type = std::unordered_map<std::string, std::chrono::milliseconds>;

int main(int argc, char** argv)
{
    if(argc < 3 || argc > 5)
    {
        std::cout
            << "Usage: " << argv[0] << " <username> <channel> [host] [port]\n"
			<< "\t<username>: Self-assigned username in channel.\n"
			<< "\t<channel>: Channel to join.\n"
            << "\t<host>: Host of the bootstrap node (default: "
				<< default_bootstrap_host << ").\n"
            << "\t<port>: Port of the bootstrap node (default: "
				<< default_bootstrap_port << ").\n"
            << std::flush;
        return 1;
    }

    // Use GNU readline history.
    using_history();

    // Create an unordered_map, for messages deduplication.
    map_type map;

    // Use a opendht node.
    dht::DhtRunner node;

    // Create a brand new identity.
    auto node_identity = dht::crypto::generateIdentity();

    // Run a node with a free port (0), use a separate thread.
    node.run(0, node_identity, true);

    // Join the network through an already known node.
    node.bootstrap(
			argc >= 4 ? argv[3] : default_bootstrap_host,
			argc >= 5 ? argv[4] : default_bootstrap_port);

    // User settings.
    msg::username = argv[1];
    std::string chan = argv[2];

    // Pack&Put data on the DHT.
    node.putSigned(chan, msg("Enter the chatroom."));

    // Retrieve past, old unordered messages, ignore-them.
    node.get(chan,
        [&](const std::vector<std::shared_ptr<dht::Value>>& values)
        {
            // For every value found...
            for (const auto& value : values)
            {
                // Unpack then register the value.
                std::string content = value->unpack<std::string>();
                map.insert(std::make_pair(content, get_timestamp()));
            }

            // Continue lookup until no values are left.
            return true;
        });

    // Watch for incoming new messages.
    node.listen(chan,
        [&](const std::vector<std::shared_ptr<dht::Value>>& values)
        {
            // For every value found...
            for (const auto& value : values)
            {
                // Unpack then register and display it, if it's a new value.
                std::string content = value->unpack<std::string>();
                if(!map.count(content))
                {
                    map.insert(std::make_pair(content, get_timestamp()));
                    disp(content);
                }
            }

            // Continue lookup until no values are left.
            return true;
        });

    // Read stdin and put messages on the DHT.
    for(;;)
    {
		bool eof = false;
        std::string output = input("", eof);
		if(eof)
			break;

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

std::chrono::milliseconds get_timestamp()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch());
}

void disp(std::string content)
{
    std::cout << content << std::endl;
}

std::string input(const std::string& prompt, bool& eof)
{
    const char* line = readline(prompt.c_str());
    if(line && *line)
        add_history(line);

    if(line)
        return std::string(line);
	else {
		eof = true;
		return std::string("");
	}
}

