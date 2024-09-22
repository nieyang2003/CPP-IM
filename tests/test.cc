#include <nlohmann/json.hpp>
#include <cpr/cpr.h>
#include <gtest/gtest.h>
#include <gflags/gflags.h>
#include <uWebSockets/App.h>
#include <google/protobuf/text_format.h>
#include "thirdparty/easywsclient/easywsclient.hpp"
#include "proto/msg.pb.h"

DEFINE_bool(test_get_varify_code, false, "");
DEFINE_bool(test_register, false, "");
DEFINE_string(account1_email, "3357249182@qq.com", "测试账号 1 的邮箱");
DEFINE_string(account1_pass,  "123456", "测试账号 1 的密码");
DEFINE_string(account2_email, "3536448549@qq.com", "测试账号 2 的邮箱");
DEFINE_string(account2_pass,  "123456", "测试账号 2 的邮箱");

std::string code; // 验证码
std::string atoken, rtoken, token; // gate、http
std::string auth_header;

using easywsclient::WebSocket;

void Login(nlohmann::json *resp_body,
           const std::string& email = FLAGS_account1_email,
           const std::string& pass  = FLAGS_account1_pass) {
    nlohmann::json req_body = {
        {"email", email},
        {"password", pass},
    };
    auto resp = cpr::Post(
        cpr::Url{"http://127.0.0.1:8080/login"},
        cpr::Header{{"Content-Type", "application/json"}},
        cpr::Body{req_body.dump()}
    );
    ASSERT_EQ(resp.status_code, 200);
    *resp_body = nlohmann::json::parse(resp.text);
    ASSERT_EQ((*resp_body)["code"], 1000);
}

void LoginGate1(WebSocket::pointer* ws,
               const std::string& email = FLAGS_account1_email,
               const std::string& pass  = FLAGS_account1_pass) {
    nlohmann::json resp_json;
    Login(&resp_json, email, pass);
    nlohmann::json msg_json = resp_json["msg"];

    std::string uid = msg_json["uid"];
    std::string token  = msg_json["token"];
    std::string location  = msg_json["msg_server_location"];
    std::string url = "ws://" + location + "/msg?uid=" + uid + "&token=" + token;
    /* ws://x.x.x.x:port/msg?uid=33504309278670849&token=NqQBTYmrMa */
    std::cout << url << std::endl;

    *ws = WebSocket::from_url(url);
    ASSERT_NE(*ws, nullptr);
}

void LoginGate2(WebSocket::pointer* ws,
                const std::string& email = FLAGS_account2_email,
                const std::string& pass  = FLAGS_account2_pass) {
    LoginGate1(ws, email, pass);
}

TEST(login, GetVarifyCode) {
    if (!FLAGS_test_get_varify_code && !FLAGS_test_register) {
        return;
    }

    std::cout << "发送给邮箱：" << FLAGS_account1_email;
    nlohmann::json req_body = {
        {"email", FLAGS_account1_email}
    };
    cpr::Response r = cpr::Post(
        cpr::Url{"http://127.0.0.1:8080/get_varify_code"},
        cpr::Header{{"Content-Type", "application/json"}},
        cpr::Body{req_body.dump()}
    );
    std::cout << r.text << std::endl;
    ASSERT_EQ(r.status_code, 200);
}

TEST(login, Register) {
    if (!FLAGS_test_register) {
        return;
    }
    std::cout << "请输入验证码：";
    std::cin >> code;

    nlohmann::json req_body = {
        {"username", "test_account"},
        {"email", FLAGS_account1_email},
        {"password", FLAGS_account1_pass},
        {"confirm", FLAGS_account1_pass},
        {"sex", 0},
        {"varify_code", code}
    };
    cpr::Response r = cpr::Post(
        cpr::Url{"http://127.0.0.1:8080/register"},
        cpr::Header{{"Content-Type", "application/json"}},
        cpr::Body{req_body.dump()}
    );
    std::cout << r.text << std::endl;
    ASSERT_EQ(r.status_code, 200);
}

TEST(login, login) {
    nlohmann::json resp_json;
    Login(&resp_json);
    ASSERT_TRUE(resp_json.contains("msg"));
    nlohmann::json msg_json = resp_json["msg"];
    ASSERT_TRUE(msg_json.contains("access_token"));
    ASSERT_TRUE(msg_json.contains("refresh_token"));
    ASSERT_TRUE(msg_json.contains("token"));

    atoken = msg_json["access_token"];
    rtoken = msg_json["refresh_token"];
    token  = msg_json["token"];
    auth_header = "Bearer " + atoken;
}

TEST(login, refresh_token) {
    std::this_thread::sleep_for(std::chrono::seconds(1));

    cpr::Response r = cpr::Get(
        cpr::Url{"http://127.0.0.1:8080/refresh_token"},
        cpr::Header{{"Content-Type", "application/json"},
                    {"Authorization", auth_header}},
        cpr::Parameters{{"refresh_token", rtoken}}
    );
    std::cout << r.text << std::endl;
    ASSERT_EQ(r.status_code, 200);
}

TEST(login, auth) {
    // 正确token
    cpr::Response r = cpr::Get(
        cpr::Url{"http://127.0.0.1:8080/friend"},
        cpr::Header{{"Content-Type", "application/json"},
                    {"Authorization", auth_header}}
    );
    std::cout << r.text << std::endl;
    nlohmann::json resp_json = nlohmann::json::parse(r.text);
    ASSERT_EQ(resp_json["code"], 1000);
    // 错误的Token
    r = cpr::Get(
        cpr::Url{"http://127.0.0.1:8080/friend"},
        cpr::Header{{"Content-Type", "application/json"},
                    {"Authorization", "error token"}}
    );
    std::cout << r.text << std::endl;
    resp_json = nlohmann::json::parse(r.text);
    ASSERT_NE(resp_json["code"], 1000);
    // 错误的Token2
    r = cpr::Get(
        cpr::Url{"http://127.0.0.1:8080/friend"},
        cpr::Header{{"Content-Type", "application/json"},
                    {"Authorization", "Bearer errortoken"}}
    );
    std::cout << r.text << std::endl;
    resp_json = nlohmann::json::parse(r.text);
    ASSERT_NE(resp_json["code"], 1000);
}

TEST(Msg, login) {
    WebSocket::pointer ws1;
    LoginGate1(&ws1);
    ws1->close();
    delete ws1;

    WebSocket::pointer ws2;
    LoginGate2(&ws2);
    ws2->close();
    delete ws2;
}

TEST(Msg, send) {
    bool user2_logged = false;
    std::mutex mtx;
    std::condition_variable cv;
    std::unique_lock<std::mutex> lock(mtx);

    std::thread friend_thread([&](){
        WebSocket::pointer ws2;
        LoginGate2(&ws2); // 账号2
        bool quit = false;
        user2_logged = true;
        cv.notify_one();

        msg::MqMessage msg;
        while (ws2->getReadyState() != WebSocket::CLOSED) {
            ws2->poll();
            ws2->dispatchBinary([&](const std::vector<unsigned char>& message) {
                std::cout << "收到实时消息: " <<  message.size() << " bytes" << std::endl;
                ASSERT_TRUE(msg.ParseFromArray(message.data(), message.size()));
                quit = true;
            });
            if (quit) break;
        }
        std::string text;
        google::protobuf::TextFormat::PrintToString(msg, &text);
        std::cout << text << std::endl;
        delete ws2;
        return;
    });

    
    WebSocket::pointer ws1;
    LoginGate1(&ws1); // 账号1
    bool recived = false;

    msg::ProtoReq req;
    req.set_type(msg::ProtoReqType::TypeFriendCtrl);
    req.set_from_uid(34846162674515969);
    req.set_to_id(34877991586627585);
    req.mutable_fctrl()->set_type(msg::FriendCtrl::FRIEND_REQ); // 添加好友
    auto data = req.SerializeAsString();

    // 等待账号2上线
    cv.wait(lock, [&](){ return user2_logged; });
    ws1->sendBinary(data);

    msg::ProtoResp resp;
    while (ws1->getReadyState() != WebSocket::CLOSED) {
        ws1->poll();
        ws1->dispatchBinary([&](const std::vector<unsigned char>& message) {
            std::cout << "receive " <<  message.size() << " bytes" << std::endl;
            ASSERT_TRUE(resp.ParseFromArray(message.data(), message.size()));
            recived = true;
        });
        if (recived) break;
    }
    std::string text;
    google::protobuf::TextFormat::PrintToString(resp, &text);
    std::cout << "发送的响应：" << text << std::endl;
    ASSERT_EQ(resp.has_timestamp(), true);

    delete ws1;
    std::cout << "等待线程" << std::endl;
    friend_thread.join();
}

int main(int argc, char **argv) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}