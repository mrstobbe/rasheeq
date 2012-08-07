#ifndef Rasheeq_Poller_H_
#define Rasheeq_Poller_H_

#ifdef __cplusplus
namespace R {

class Poller {
	private:
		#if RASHEEQ_HAVE_EPOLL
		int efd_;
		#endif
		long timeout_;
	public:
		Poller();
		~Poller();
	public:
		inline void poll() { this->poll(this->timeout_); };
		void poll(const long timeout);
}; //class Poller

}; //ns R

extern "C" {
#endif /* __cplusplus */

void rasheeq_poller_poll(R::Poller* poller);
void rasheeq_poller_timed_poll(R::Poller* poller, const long timeout);

#ifdef __cplusplus
} //extern "C"
#endif /* __cplusplus */


#endif /* Rasheeq_Poller_H_ */
