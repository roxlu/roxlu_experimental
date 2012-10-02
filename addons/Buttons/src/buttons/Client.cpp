#include <buttons/Buttons.h>
#include <buttons/Client.h>
#include <buttons/Server.h>

namespace buttons {
	Client::Client(const std::string ip, int port)
		:ip(ip)
		,port(port)
	{
	}

	Client::~Client() {
	}

	void Client::start() {
		thread.create(*this);
	}

	// RUN IN SEPARATE THREAD
	void Client::run() {
		printf("running?\n");
		if(!sock.connect(ip.c_str(), port)) {
			printf("Error: cannot sonnect to %s:%d\n", ip.c_str(), port);
			return;
		}
		
		std::vector<char> tmp(2048);
		sock.setBlocking(false);
		while(true) {
			int bytes_read = sock.read(&tmp[0], tmp.size());
			if(bytes_read) {
				buffer.addBytes((char*)&tmp[0], bytes_read);
				parseBuffer();
			}
			// Check if there are tasks that we need to handle
			mutex.lock();
			{
				for(std::vector<ClientTask>::iterator it = out_tasks.begin(); it != out_tasks.end(); ++it) {
					ClientTask& task = *it;
					send(task.buffer.getPtr(), task.buffer.getNumBytes());
					/*
					switch(task.name) {
					case BCLIENT_SEND_TO_SERVER: {

						break;
					}
					default: {
						break;
					}}
					*/
				}
				out_tasks.clear();
			}
			mutex.unlock();
		}
	}
	void Client::send(const char* buffer, size_t len) {
		size_t bytes_left = len;
		sock.setBlocking(true);
		while(bytes_left) {
			int done = sock.send(buffer, len);
			if(done < 0) {
				printf("Error: handle this error... \n");
				break;
			}
			else {
				bytes_left -= done;
				printf("Left to send: %zu\n", bytes_left);
			}
		}
		sock.setBlocking(false);
	}

	// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++	
	void Client::parseBuffer() {
		if(!buffer.size()) {
			return;
		}
		int command_name = buffer[0];
	
		switch(command_name) {
		case BSERVER_SCHEME: parseCommandScheme(); break;
		case BSERVER_COMMAND_TEST: parseCommandTest(); break;
		case BSERVER_VALUE_CHANGED: parseCommandValueChanged(); break;
		default: break;
		};

	}
	
	void Client::parseCommandTest() {
		size_t size = 0;
		if(buffer.size() < sizeof(size)) {
			return ;
		}
	}

	// GET SCHEME BUFFER
	void Client::parseCommandScheme() {
		// DID WE RECEIVE ENOUGH DATA?
		if(buffer.size() < 5 || (buffer.size() > 0 &&  buffer[0] != BSERVER_SCHEME)) {
			printf("Error: cannot parse scheme; incorrect data.\n");
			return;
		}
		unsigned int scheme_size = buffer.getUI32(1);
		if(buffer.size() < scheme_size) {
			return;
		}

		// CREATE TASK TO PARSE THE SCHEME IN MAIN THREAD
		ButtonsBuffer task_buffer;
		buffer.flush(5);
		task_buffer.addBytes(buffer.getPtr(), buffer.getNumBytes());
		ClientTask task(BCLIENT_PARSE_SCHEME);
		task.buffer = task_buffer;
		addInTask(task);
		buffer.flush(task_buffer.size());
	}

	void Client::parseCommandValueChanged() {
		// DID WE GET ENOUGH DATA?
		if(buffer.size() < 5 || buffer[0] != BSERVER_VALUE_CHANGED) {
			return;
		}
		unsigned int size = buffer.getUI32(1);
		if(buffer.size() < size) {
			printf("<< Buffer too small, we have: %u and need: %u\n", buffer.size(), size);
			return;
		}
		printf("<< Buffer size: %zu and we needed: %u\n", buffer.size(), size);
		buffer.flush(5);

		// GET ELEMENT
		unsigned int buttons_id = buffer.consumeUI32();
		unsigned int element_id = buffer.consumeUI32();
		Element* el = elements[buttons_id][element_id];
		
		// PARSE ELEMENT TYPE
		switch(el->type) {
		case BTYPE_SLIDER: {
			Sliderf* sliderf = static_cast<Sliderf*>(el);
			if(sliderf->value_type == Slider<float>::SLIDER_FLOAT) {
				float element_value = buffer.consumeFloat();
				ClientTask ctask(BCLIENT_VALUE_CHANGED_SLIDERF);
				ctask.element = el;
				ctask.sliderf = sliderf;   // @todo figure out if this might lead to threading problems..
				ctask.sliderf_value = element_value;
				addInTask(ctask);
			}
			break;
		}
		default: printf("Error: unhandled sync type: %d\n", el->type); break;
		}
		printf("Bytes left: %zu\n", buffer.size());
	}

	// We received data from the server; add it to the "in task" list. (e.g. changing values(
	void Client::addInTask(ClientTask task) {
		mutex.lock();
		{
			in_tasks.push_back(task);
		}
		mutex.unlock();
	}

	// We changed the gui/values so update the remote server (out task)
	void Client::addOutTask(ClientTask task) {
		mutex.lock();
		{
			out_tasks.push_back(task);
		}
		mutex.unlock();
	}

	// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	void Client::update() {
		mutex.lock();
		{
			// Parse new tasks.
			std::vector<ClientTask>::iterator it = in_tasks.begin(); 
			while(it != in_tasks.end()) {
				ClientTask& task = *it;
				switch(task.name) {
				case BCLIENT_VALUE_CHANGED_SLIDERF: {
					task.sliderf->setValue(task.sliderf_value); 
					task.sliderf->needsRedraw();
					break;
				}
				case BCLIENT_PARSE_SCHEME: {
					parseTaskScheme(task);
					break;
				}
				default: {
					printf("Error: Unhandled task.\n"); break;
					break;
				}}
				it = in_tasks.erase(it);
			}
		}
		mutex.unlock();
		for(std::map<unsigned int, buttons::Buttons*>::iterator it = buttons.begin(); it != buttons.end(); ++it) {
			it->second->update();
		}
	}

	void Client::draw() {
		for(std::map<unsigned int, buttons::Buttons*>::iterator it = buttons.begin(); it != buttons.end(); ++it) {
			it->second->draw();
		}
	}

	void Client::onMouseMoved(int x, int y) {
		for(std::map<unsigned int, buttons::Buttons*>::iterator it = buttons.begin(); it != buttons.end(); ++it) {
			it->second->onMouseMoved(x,y);
		}
	}

	void Client::onMouseUp(int x, int y) {
		for(std::map<unsigned int, buttons::Buttons*>::iterator it = buttons.begin(); it != buttons.end(); ++it) {
			it->second->onMouseUp(x,y);
		}
	}

	void Client::onMouseDown(int x, int y) {
		for(std::map<unsigned int, buttons::Buttons*>::iterator it = buttons.begin(); it != buttons.end(); ++it) {
			it->second->onMouseDown(x,y);
		}
	}
	
	
	// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// CREATES THE BUTTONS/PANELS FROM THE REMOTE SCHEME
	void Client::parseTaskScheme(ClientTask& task) {
		while(true) {
			char scheme_cmd = task.buffer.consumeByte();
			switch(scheme_cmd) {
			case BSCHEME_GUI: {
				unsigned int x = task.buffer.consumeUI32();
				unsigned int y = task.buffer.consumeUI32();
				unsigned int w = task.buffer.consumeUI32();
				unsigned int h = task.buffer.consumeUI32();
				std::string title = task.buffer.consumeString();
				unsigned short num_els = task.buffer.consumeUI16();
				
				// STORE AT ID
				buttons::Buttons* gui = new Buttons(title, w);
				gui->addListener(this);
				unsigned int buttons_id = buttons_hash(title.c_str(), title.size());
				buttons[buttons_id] = gui;

				// PARSE ALL ELEMENTS
				for(int i = 0; i < num_els; ++i) {

					// COMMON/SHARED VALUES
					char el_type = task.buffer.consumeByte();
					std::string label = task.buffer.consumeString();
					unsigned int element_id = buttons_hash(label.c_str(), label.size());
					float col_hue = task.buffer.consumeFloat();
					float col_sat = task.buffer.consumeFloat();
					float col_bright = task.buffer.consumeFloat();

					switch(el_type) {
					case BTYPE_SLIDER: {
						int value_type = task.buffer.consumeByte();
						
						if(value_type == Slider<float>::SLIDER_FLOAT) {
							float value = task.buffer.consumeFloat();
							float minv = task.buffer.consumeFloat();
							float maxv = task.buffer.consumeFloat();
							float stepv = task.buffer.consumeFloat();
							float* dummy_value = new float;
							value_floats.push_back(dummy_value);							

							// CREATE SLIDER
							Sliderf* sliderp = &gui->addFloat(label, *dummy_value).setMin(minv).setMax(maxv).setStep(stepv);
							sliderp->setValue(value);
							gui->setColor(col_hue);

							elements[buttons_id][element_id] = sliderp;
						}
						break;
					}
					default: {
						printf("Error: Unhandled scheme type: %d\n", el_type);
						break;
					}}
				}
				break; // fror now we break after one el.
			}
			default:break;
			}
			break;
		}
	}

	// From Client > Server
	// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	void Client::addSendToServerTask(ClientTaskName name, ButtonsBuffer buffer) {
		ClientTask task(name);
		task.buffer.addByte(name);
		task.buffer.addUI32(buffer.getNumBytes());
		task.buffer.addBytes(buffer.getPtr(), buffer.getNumBytes());
		addOutTask(task);
	}

	void Client::onEvent(ButtonsEventType event, const Buttons& buttons, const Element* target) {
		// @todo some duplicate code here, see Server::onEvent; maybe move this to the ClientServerUtils
		if(event == BEVENT_VALUE_CHANGED) {
			ButtonsBuffer buffer;
			if(utils.serializeOnValueChanged(buttons, target, buffer)) {
				printf("Created a buffer: %zu\n", buffer.size());
				addSendToServerTask(BCLIENT_VALUE_CHANGED, buffer);
				//connections.sendToAll(cmd);
			}
		}
	}

} // buttons
