#include "notcurses.capnp.h"

#include <capnp/rpc-twoparty.h>

#include <kj/async-io.h>

struct KeypressHandler
  : Input::Server {

  kj::Promise<void> keypress(KeypressContext ctx) override {
    auto params = ctx.getParams();
    auto key = params.getKey();
    KJ_LOG(ERROR, "Key pressed", key);
    return kj::READY_NOW;
  }
};

int main(int argc, char** argv) {

  auto ioCtx = kj::setupAsyncIo();
  auto& waitScope = ioCtx.waitScope;
  auto& network = ioCtx.provider->getNetwork();
  auto& timer = ioCtx.provider->getTimer();

  auto addr = network.parseAddress("0.0.0.0:9876").wait(waitScope);
  auto stream = addr->connect().wait(waitScope);
  capnp::TwoPartyClient client{*stream};
  auto nc = client.bootstrap().castAs<Notcurses>();
  auto req = nc.rootRequest();
  req.setInput(kj::heap<KeypressHandler>());
  auto plane = req.send().wait(waitScope).getPlane();

  {
    auto req = plane.putTextRequest();
    req.setText("Hello, world!"_kj);
    auto reply = req.send().wait(waitScope);
    KJ_LOG(ERROR, reply);
  }
  {
    auto req = plane.putTextRequest();
    req.setText("Hello, world!"_kj);
    req.setY(1);
    auto reply = req.send().wait(waitScope);
    KJ_LOG(ERROR, reply);
  }
  {
    auto req = plane.putTextRequest();
    req.setText("Hello, world!"_kj);
    req.setY(2);
    auto reply = req.send().wait(waitScope);
    KJ_LOG(ERROR, reply);
  }
  {
    auto req = plane.putTextRequest();
    req.setText("Hello, world!"_kj);
    req.setY(3);
    auto reply = req.send().wait(waitScope);
    KJ_LOG(ERROR, reply);
  }

  {
    auto req = plane.putStrRequest();
    req.setText("Hello, world!"_kj);
    req.setY(5);
    req.setX(2);
    auto reply = req.send().wait(waitScope);
    KJ_LOG(ERROR, reply);
  }

  
  kj::NEVER_DONE.wait(waitScope);
  return 0;
}
