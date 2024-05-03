#define CATCH_CONFIG_RUNNER

#include <catch2/catch_all.hpp>

// Pretty-printing for `std::optional<std::system_error>`
namespace Catch {
template<> struct StringMaker<std::optional<std::system_error>> {
  static std::string convert(std::optional<std::system_error> const& maybeErr) {
    if (!maybeErr) { return "No error"; }
    const auto& e = maybeErr.value();
    const std::string category = e.code().category().name();
    const std::string code = std::to_string(e.code().value());
    return std::string{e.what()} + " [" + category + ": " + code + "]";
  }
};
} // namespace Catch

int main(int argc, char* argv[]) {
  return Catch::Session().run(argc, argv);
}
