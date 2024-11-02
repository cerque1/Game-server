#include "sdk.h"
#include "log_utils.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/json.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <thread>

#include "json_loader.h"
#include "request_handler.h"

using namespace std::literals;
namespace net = boost::asio;
namespace logging = boost::log;

using Strand = boost::asio::strand<boost::asio::io_context::executor_type>;

namespace {

class Ticker : public std::enable_shared_from_this<Ticker> {
public:
    using Strand = net::strand<net::io_context::executor_type>;
    using Handler = std::function<void(std::chrono::milliseconds delta)>;

    // Функция handler будет вызываться внутри strand с интервалом period
    Ticker(Strand strand, std::chrono::milliseconds period, Handler handler)
        : strand_{strand}
        , period_{period}
        , handler_{std::move(handler)} {
    }

    void Start() {
        net::dispatch(strand_, [self = shared_from_this()] {
            self->last_tick_ = Clock::now();
            self->ScheduleTick();
        });
    }

private:
    void ScheduleTick() {
        assert(strand_.running_in_this_thread());
        timer_.expires_after(period_);
        timer_.async_wait([self = shared_from_this()](boost::system::error_code ec) {
            self->OnTick(ec);
        });
    }

    void OnTick(boost::system::error_code ec) {
        using namespace std::chrono;
        assert(strand_.running_in_this_thread());

        if (!ec) {
            auto this_tick = Clock::now();
            auto delta = duration_cast<milliseconds>(this_tick - last_tick_);
            last_tick_ = this_tick;
            try {
                handler_(delta);
            } catch (...) {
            }
            ScheduleTick();
        }
    }

    using Clock = std::chrono::steady_clock;

    Strand strand_;
    std::chrono::milliseconds period_;
    net::steady_timer timer_{strand_};
    Handler handler_;
    std::chrono::steady_clock::time_point last_tick_;
};

struct Args{
    int milliseconds;
    std::string config_file;
    std::string root_path; 
    bool is_random_generate;
};

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]){
    namespace po = boost::program_options;

    po::options_description desc{"All options"s};

    Args args;
    desc.add_options()
        ("help,h", "produce help message")
        ("tick-period,t", po::value(&args.milliseconds)->value_name("milliseconds"), "set tick period")
        ("config-file,c", po::value(&args.config_file)->value_name("file"), "set config file path")
        ("www-root,w", po::value(&args.root_path)->value_name("dir"), "set static files root")
        ("randomize-spawn-points", "spawn dogs at random positions");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    // Выводим описание параметров программы
    if (vm.contains("help"s)) {
        std::cout << desc;
        return std::nullopt;
    }
    if (!vm.contains("tick-period"s)) {
        args.milliseconds = 0;
    }
    if (!vm.contains("config-file"s)) {
        throw std::runtime_error("Config-file path is not specified"s);
    }
    if (!vm.contains("www-root"s)) {
        throw std::runtime_error("Www-root file path is not specified"s);
    }

    if (!vm.contains("randomize-spawn-points"s)) {
        args.is_random_generate = false;
    }
    else{
        args.is_random_generate = true;
    }

    return args;
}

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned num_workers, const Fn& fn) {
    num_workers = std::max(1u, num_workers);
    std::vector<std::jthread> workers;
    workers.reserve(num_workers - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--num_workers) {
        workers.emplace_back(fn);
    }
    fn();
}

}  // namespace

int main(int argc, const char* argv[]) {
    try {
        if(auto args = ParseCommandLine(argc, argv)){
            // 1. Загружаем карту из файла и построить модель игры
            model::Game game = json_loader::LoadGame(fs::path(args->config_file));

            // 2. Инициализируем io_context
            const unsigned num_threads = std::thread::hardware_concurrency();
            net::io_context ioc(num_threads);

            // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
            net::signal_set signal(ioc, SIGINT, SIGTERM);
            signal.async_wait([&ioc](const boost::system::error_code& ec, [[maybe_unused]] int signal_number){
                if(!ec){
                    BOOST_LOG_TRIVIAL(info) << logging::add_value(timestamp, boost::posix_time::second_clock::local_time())
                                    << logging::add_value(additional_data, log_data::MakeServerExitedData(0))
                                    << "server exited";
                    ioc.stop();
                }
            });

            // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
            Strand api_strand = boost::asio::make_strand(ioc);

            std::shared_ptr<http_handler::RequestHandler> handler = std::make_shared<http_handler::RequestHandler>(game, fs::path(args->root_path), api_strand, args->milliseconds, args->is_random_generate);

            if(args->milliseconds > 0){
                auto ticker = std::make_shared<Ticker>(api_strand, std::chrono::milliseconds(args->milliseconds), [&handler](std::chrono::milliseconds delta){
                    handler->Tick(delta.count());
                });
                ticker->Start();
            }

            const net::ip::address address{net::ip::make_address_v4("0.0.0.0")};
            static const int port = 8080;

            // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
            http_server::ServeHttp(ioc, {address, port}, [&handler](auto&& req, auto&& send) {
                (*handler)(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
            });

            // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
            logging::add_console_log(std::clog, keywords::format = &formatter::JsonFormatter);

            BOOST_LOG_TRIVIAL(info) << logging::add_value(timestamp, boost::posix_time::second_clock::local_time())
                                    << logging::add_value(additional_data, log_data::MakeStartServerData(port, address))
                                    << "server started";

            // 6. Запускаем обработку асинхронных операций
            RunWorkers(std::max(1u, num_threads), [&ioc] {
                ioc.run();
            });
        }
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
}
