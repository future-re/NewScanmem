#include "newscanmem/cli/command.hpp"

namespace cli {
auto Command::getAliases() const -> std::vector<std::string_view> { return {}; }
auto Command::validateArgs(const std::vector<std::string>&) const -> std::expected<void, std::string> { return {}; }
auto CommandRegistry::instance() -> CommandRegistry& { static CommandRegistry registry; return registry; }
void CommandRegistry::registerCommand(std::unique_ptr<Command> command) { if (!command) return; const auto name = std::string(command->getName()); for (const auto alias : command->getAliases()) m_aliases[std::string(alias)] = name; m_commands[name] = std::move(command); }
auto CommandRegistry::findCommand(const std::string_view name) -> Command* { auto key = std::string(name); if (const auto alias = m_aliases.find(key); alias != m_aliases.end()) key = alias->second; const auto found = m_commands.find(key); return found == m_commands.end() ? nullptr : found->second.get(); }
auto CommandRegistry::getAllCommands() const -> std::vector<Command*> { std::vector<Command*> commands; commands.reserve(m_commands.size()); for (const auto& [name, command] : m_commands) commands.push_back(command.get()); return commands; }
void CommandRegistry::clear() { m_commands.clear(); m_aliases.clear(); }
auto CommandRegistry::size() const -> std::size_t { return m_commands.size(); }
auto parseCommandLine(const std::string_view line) -> std::pair<std::string, std::vector<std::string>> { std::vector<std::string> tokens; std::string current; bool quoted = false; char quote{}; for (const auto character : line) { if (character == '\'' || character == '"') { if (!quoted) { quoted = true; quote = character; continue; } if (quote == character) { quoted = false; quote = {}; continue; } } if (!quoted && std::isspace(static_cast<unsigned char>(character)) != 0) { if (!current.empty()) { tokens.push_back(std::move(current)); current.clear(); } continue; } current += character; } if (!current.empty()) tokens.push_back(std::move(current)); if (tokens.empty()) return {{}, {}}; return {tokens.front(), {tokens.begin() + 1, tokens.end()}}; }
}  // namespace cli
