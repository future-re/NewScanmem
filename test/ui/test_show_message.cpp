// Unit tests for ui::MessagePrinter
#include <gtest/gtest.h>

#include <sstream>
#include <string>

import ui.show_message;

using ui::MessageContext;
using ui::MessagePrinter;
using ui::MessageType;

// Helper to capture std::cout and std::cerr
class StreamCapture {
   public:
    StreamCapture() : oldCout(std::cout.rdbuf()), oldCerr(std::cerr.rdbuf()) {
        std::cout.rdbuf(coutBuf.rdbuf());
        std::cerr.rdbuf(cerrBuf.rdbuf());
    }
    ~StreamCapture() {
        std::cout.rdbuf(oldCout);
        std::cerr.rdbuf(oldCerr);
    }
    [[nodiscard]] auto out() const -> std::string { return coutBuf.str(); }
    [[nodiscard]] auto err() const -> std::string { return cerrBuf.str(); }

   private:
    std::stringstream coutBuf;
    std::stringstream cerrBuf;
    std::streambuf* oldCout;
    std::streambuf* oldCerr;
};

TEST(ShowMessageTest, InfoWarnErrorColored) {
    MessageContext ctx;
    ctx.colorMode = true;
    MessagePrinter printer(ctx);
    StreamCapture cap;

    printer.info("hello {}", 1);
    printer.warn("world {}", 2);
    printer.error("oops {}", 3);

    auto err = cap.err();
    EXPECT_NE(err.find("info:"), std::string::npos);
    EXPECT_NE(err.find("warn:"), std::string::npos);
    EXPECT_NE(err.find("error:"), std::string::npos);
}

TEST(ShowMessageTest, DebugRespectsFlag) {
    MessageContext ctx;
    ctx.colorMode = false;  // simplify matching
    ctx.debugMode = false;
    MessagePrinter printer(ctx);
    StreamCapture cap;

    printer.debug("no show {}", 7);
    EXPECT_EQ(cap.err().find("debug:"), std::string::npos);

    // Enable debug
    MessageContext ctx2;
    ctx2.colorMode = false;
    ctx2.debugMode = true;
    MessagePrinter printer2(ctx2);
    StreamCapture cap2;
    printer2.debug("visible {}", 8);
    EXPECT_NE(cap2.err().find("debug:"), std::string::npos);
}

TEST(ShowMessageTest, UserGoesToStdoutUnlessBackend) {
    MessageContext ctx;
    ctx.backendMode = false;
    ctx.colorMode = false;
    MessagePrinter printer(ctx);
    StreamCapture cap;
    printer.user("hi {}", 9);
    EXPECT_NE(cap.out().find("hi 9"), std::string::npos);
    EXPECT_TRUE(cap.err().empty());

    // Backend mode suppresses user output
    MessageContext backendCtx;
    backendCtx.backendMode = true;
    backendCtx.colorMode = false;
    MessagePrinter backendPrinter(backendCtx);
    StreamCapture cap2;
    backendPrinter.user("hidden {}", 10);
    EXPECT_TRUE(cap2.out().empty());
}

TEST(ShowMessageTest, StaticConvenienceHaveMarkers) {
    StreamCapture cap;
    MessagePrinter::info("hello");
    MessagePrinter::warn("care");
    MessagePrinter::error("bad");
    MessagePrinter::success("ok");
    auto err = cap.err();
    EXPECT_NE(err.find("info:"), std::string::npos);
    EXPECT_NE(err.find("warn:"), std::string::npos);
    EXPECT_NE(err.find("error:"), std::string::npos);
    EXPECT_NE(err.find("success:"), std::string::npos);
}
