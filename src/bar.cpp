#include "notcurses.capnp.h"

#include <capnp/rpc-twoparty.h>

#include <kj/async-io.h>
#include <kj/async-unix.h>
#include <kj/refcount.h>
#include <notcurses/notcurses.h>

struct NotcursesServer;

struct PlaneServer
  : Plane::Server {

  PlaneServer(kj::Own<NotcursesServer> nc, ncplane* plane);

  kj::Promise<void> putText(PutTextContext) override;
  kj::Promise<void> putStr(PutStrContext) override;

  kj::Own<NotcursesServer> nc_;
  ncplane* plane_;
};

struct NotcursesServer
  : Notcurses::Server
  , kj::Refcounted
  , kj::TaskSet::ErrorHandler {
  NotcursesServer(kj::Timer& timer, kj::UnixEventPort::FdObserver& fd, notcurses* nc)
    : timer_{timer}
    , fd_{fd}
    , nc_{nc} {

    tasks_.add(awaitInput());
  }

  kj::Own<NotcursesServer> addRef() {
    return kj::addRef(*this);
  }

  void taskFailed(kj::Exception&& exc) {
  }

  kj::Promise<void> handleInput(uint32_t keypress, ncinput* ni) {
    try {
      auto req = inputHandler_.keypressRequest();
      req.setKey(keypress);
      return req.send().ignoreResult();
    }
    catch (kj::Exception& exc) {
      KJ_LOG(ERROR, exc);
    }
    return kj::READY_NOW;
  }

  kj::Promise<void> root(RootContext ctx) override {
    auto params = ctx.getParams();
    auto inputHandler = params.getInput();
    inputHandler_ = inputHandler;
    auto reply = ctx.getResults();
    reply.setPlane(kj::heap<PlaneServer>(addRef(), notcurses_stdplane(nc_)));
    return kj::READY_NOW;
  }

  kj::Promise<void> awaitInput() {
    KJ_DEFER(notcurses_render(nc_));
    return fd_.whenBecomesReadable().then([this]{
      ncinput ni;
      while (true) {
        auto keypress = notcurses_get_nblock(nc_, &ni);
        if (keypress == 0) {
          break;
        }
        tasks_.add(handleInput(keypress, &ni));
      }

      return awaitInput();
    });
  }

  void refresh() {
    notcurses_render(nc_);
  }

  kj::Timer& timer_;
  kj::UnixEventPort::FdObserver& fd_;
  notcurses* nc_;
  Input::Client inputHandler_{nullptr};
  kj::TaskSet tasks_{*this};
};


PlaneServer::PlaneServer(kj::Own<NotcursesServer> nc, ncplane* plane)
  : nc_{kj::mv(nc)}
  , plane_{plane} {
}


kj::Promise<void> PlaneServer::putText(PutTextContext ctx) {
  auto params = ctx.getParams();
  auto txt = params.getText();
  auto y = params.getY();
  std::size_t bytes;
  ncplane_puttext(plane_, y, NCALIGN_LEFT, txt.cStr(), &bytes);
  nc_->refresh();

  auto reply = ctx.getResults();
  reply.setBytes(bytes);
  return kj::READY_NOW;
}

kj::Promise<void> PlaneServer::putStr(PutStrContext ctx) {
  auto params = ctx.getParams();
  auto txt = params.getText();
  auto y = params.getY();
  auto x = params.getX();
  auto count = ncplane_putstr_yx(plane_, y, x, txt.cStr());
  KJ_REQUIRE(count >= 0);
  nc_->refresh();
  auto reply = ctx.getResults();
  reply.setCount(count);
  return kj::READY_NOW;
}

Notcurses::Client newNotcurses(kj::Timer& timer, kj::UnixEventPort::FdObserver& fd, notcurses* nc) {
  return kj::refcounted<NotcursesServer>(timer, fd, nc);
}

int main(int argc, char** argv) {

  auto ioCtx = kj::setupAsyncIo();
  auto& waitScope = ioCtx.waitScope;
  auto& network = ioCtx.provider->getNetwork();
  auto& timer = ioCtx.provider->getTimer();

  auto addr = network.parseAddress("0.0.0.0:9876").wait(waitScope);
  auto listener = addr->listen();

  notcurses_options opts{};
  auto nc = notcurses_init(&opts, nullptr);
  KJ_DEFER(notcurses_stop(nc));
  
  kj::UnixEventPort::FdObserver fd{
    ioCtx.unixEventPort,
    notcurses_inputready_fd(nc),
    kj::UnixEventPort::FdObserver::OBSERVE_READ};

  auto cap = newNotcurses(timer, fd, nc);

  capnp::TwoPartyServer server{kj::mv(cap)};
  server.listen(*listener).wait(waitScope);
}
