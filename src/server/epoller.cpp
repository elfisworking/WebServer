#include "epoller.h"
Epoller::Epoller(int maxEvent):epollFd(epoll_create(512)), events_(maxEvent) {
    assert(epollFd > 0 && events_.size() > 0);
}

Epoller::~Epoller() {
    // close fd
    close(epollFd);
}

// add epoll event
bool Epoller::addFd(int fd, uint32_t events) {
    if(fd < 0) return false;
    epoll_event event= {0};
    event.data.fd = fd;
    event.events = events;
    int state = epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &event);
    printf("%s\n", strerror(errno));
    return state == 0;
}

// modify epoll event
bool Epoller::modFd(int fd, uint32_t events) {
    if(fd < 0) return false;
    struct epoll_event event;
    event.data.fd = fd;
    event.events = events;
    int state = epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &event);
    return state == 0;
}

// delete epoll event
bool Epoller::delFd(int fd) {
    if(fd < 0) return false;
    struct epoll_event event {0};
    int state = epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, &event);
    return state == 0;
}

int Epoller::wait(int timeoutMs) {
    return epoll_wait(epollFd, &events_[0], static_cast<int>(events_.size()), timeoutMs);
}

int Epoller::getEventFd(size_t i) const {
    assert(i < events_.size() && i >= 0);
    return events_[i].data.fd;
}

uint32_t  Epoller::getEvents(size_t i) const {
    assert(i < events_.size() && i >= 0);
    return events_[i].events;
}