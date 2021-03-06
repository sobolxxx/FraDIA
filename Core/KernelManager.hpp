/*!
 * \file KernelFactory.hpp
 * \brief File containing the KernelFactory template class.
 *
 * \author tkornuta
 * \date Feb 10, 2010
 */

#ifndef KERNELMANAGER_HPP_
#define KERNELMANAGER_HPP_

#include <string>
#include <iostream>
#include <vector>
#include <dirent.h>

#include <boost/foreach.hpp>
#include <boost/ptr_container/ptr_map.hpp>

#include "boost/filesystem.hpp"
using namespace boost::filesystem;

#include "Kernel_Aux.hpp"
#include "FraDIAException.hpp"
#include "Configurator.hpp"
#include "State.hpp"
#include "Singleton.hpp"
#include "SharedLibraryCommon.hpp"

// Forward declaration of classes required by specialized template methods.
/*namespace Base {
 class Kernel_Task;
 }*/

//#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32) || defined(MSC_VER)
//#  define LIB_EXT ".dll"
//#else
//#  define LIB_EXT ".so"
//#endif

using namespace std;

namespace Core {

/*!
 * \namespace KernelManagerAux
 * \brief The KernelManagerAux namespace contains names used for passing manager name as "Template Non-Type Parameters".
 * \author tkornuta
 */
namespace KernelManagerAux {
/*!
 * Name of Sources KernelManager as well as its xml node.
 */
char Sources[] = SOURCES;

/*!
 * Name of Processors KernelManager as well as its xml node.
 */
char Processors[] = PROCESSORS;

}//: namespace KernelManagerAux


/*!
 * \class KernelManager
 * \brief
 * \author tkornuta
 */
template <class KRNL, Base::kernelType KERNEL_TYPE, char* MANAGER_NAME>
class KernelManager: public Base::State <std::string>, public Base::Singleton <KernelManager <KRNL, KERNEL_TYPE, MANAGER_NAME> >
{
	/*!
	 * Singleton class must be a friend, because only it can call protected constructor.
	 */
	friend class Base::Singleton <KernelManager <KRNL, KERNEL_TYPE, MANAGER_NAME> >;

protected:
	/*!
	 * Private constructor, called only by the Singleton::init() method.
	 */
	KernelManager()
	{
		cout << MANAGER_NAME << "Manager: Hello private \n";//<<name<<endl;
		active_kernel_factory = 0;
	}

	/*!
	 * List of kernel factories properly loaded by the manager.
	 */
	boost::ptr_map <string, KRNL> kernel_factories;

	/*!
	 * Kernel iterator
	 */
	//boost::ptr_map <string, KRNL>::const_iterator it;

	/*!
	 * Active kernel.
	 */
	KRNL* active_kernel_factory;

public:
	/*!
	 * Public destructor.
	 */
	~KernelManager()
	{
		cout << MANAGER_NAME << "Manager: Goodbye public\n";
		// Deactivate kernel.
		if (active_kernel_factory) {
			active_kernel_factory->deactivate();
			active_kernel_factory = 0;
		}
		// Kernel destructors are called automagically by ptr_map.
	}

	/*!
	 * Return active kernel
	 */
	KRNL* getActiveKernel() {
		return active_kernel_factory;
	}

	/*!
	 * Method tries to create kernels from all shared libraries loaded from the . directory.
	 */
	void initializeKernelsList()
	{
		// Retrieve node with default settings from configurator.
		xmlNodePtr tmp_node = CONFIGURATOR.returnManagerNode(KERNEL_TYPE);
		cout<<"!!returned "<<string(MANAGER_NAME)<<":node name:"<<tmp_node->name<<endl;

		// Get filenames.
		vector <string> files = vector <string> ();
		getSOList(".", files);

		// Check number of so's to import.
		if (files.size() == 0) {
			// I think, that throwing here is much to brutal
			//throw Common::FraDIAException(string(MANAGER_NAME)+string("Manager: There are no dynamic libraries in the current directory."));
			cout << string(MANAGER_NAME) << "Manager: There are no dynamic libraries in the current directory.\n";
			return;
		}

		// Iterate through so names and add retrieved kernels to list.
		BOOST_FOREACH(string file, files)
		{
			// Create kernel empty "shell".
			KRNL* k = new KRNL();
			// Try to initialize kernel.
			if (k->lazyInitialize(file))
			{
				// Retrieve configuration from config.
				xmlNodePtr node = CONFIGURATOR.returnKernelNode(KERNEL_TYPE, k->getName().c_str());
				k->ret_state()->setNode(node);
				// Add kernel to list.
				kernel_factories.insert(k->getName(), k);
			}
			else
				// Delete unpropper kernel.
				delete (k);
		}//: FOREACH

		// Check number of successfully loaded kernels.
		if (!kernel_factories.size())
			throw Common::FraDIAException(string(MANAGER_NAME)+string("Manager: There are no compatible dynamic libraries in current directory."));

		setNode(tmp_node);
		// Activate kernel.
		active_kernel_factory = kernel_factories.find(state)->second;
		cout << "Activated kernel: " << active_kernel_factory->getName() << endl;
		active_kernel_factory->activate();
	}

	void getSOList(string dir_, vector <string>& files)
	{
		cout << "LIB_EXT = " LIB_EXT << endl;
		// find all libraries in current directory
		path dir_path(dir_);
		directory_iterator end_itr; // default construction yields past-the-end
		for ( directory_iterator itr( dir_path ); itr != end_itr; ++itr )
		{
			if ( itr->path().extension() == LIB_EXT ) {
				cout << "Found library: " << itr->path().file_string() << endl;
				files.push_back(itr->path().file_string());
			}
		}
	}



	/*!
	 * Activates default kernel. Empty method - to predefine for Kernel_Source and Kernel_Task types.
	 */
/*	void selectDefaultKernel()
	{
		// TODO read which to load from the configuration file.
		cout << "STATE:" << state << endl;
		//active_kernel_factory = kernel_factories.begin()->second;


		if (kernel_factories.find(state) == kernel_factories.end()) {
			// Load first kernel and set default state.
			active_kernel_factory = kernel_factories.begin()->second;
			setDefaultState();
		} else
			active_kernel_factory = kernel_factories.find(state)->second;
		cout << "Selected: " << active_kernel_factory->getName() << endl;
		active_kernel_factory->activate();
	}*/

	/*!
	 * Loads the values of the state from the xml node.
	 */
	void loadStateFromNode(const xmlNodePtr node_)
	{
		cout<<"!!loadState:"<<string(MANAGER_NAME)<<":node name:"<<node_->name<<endl;
		state = string((char*) xmlGetProp(node_, XMLCHARCAST"default"));
		if (kernel_factories.find(state) == kernel_factories.end())
			throw Common::FraDIAException(string(MANAGER_NAME)+string("Manager: Invalid default kernel \'")+state+string("\'."));
	}

	/*!
	 * Fills the xml node basing on the values of the state.
	 */
	void saveStateToNode(xmlNodePtr& node_)
	{
		xmlSetProp(node_, XMLCHARCAST "default", XMLCHARCAST state.c_str());
	}

	/*!
	 * Fills state with default values.
	 */
	void setDefaultState()
	{
		// Set first kernel as default.
		state = kernel_factories.begin()->first;
	}

};

/*!
 * Activates default kernel. Specialized method for Kernel_Source type.
 */
/*template <>
 void Core::KernelFactory <Base::Kernel_Source>::selectDefaultKernel()
 {
 cout << "Source kernel activation\n";
 // TODO read from config.
 active_kernel = kernels.begin()->second;
 active_kernel->activate();

 }
 */
}//: namespace Core


/*!
 * \def SOURCES_MANAGER
 * \brief A macro for shorten the call to retrieve the instance of source-factories manager.
 * \author tkornuta
 * \date Mar 13, 2010
 */
#define SOURCES_MANAGER Core::KernelManager<Core::SourceFactory, Base::KERNEL_SOURCE, KernelManagerAux::Sources>::instance()

/*!
 * \def PROCESSORS_MANAGER
 * \brief A macro for shorten the call to retrieve the instance of processor-factories manager.
 * \author tkornuta
 * \date Mar 13, 2010
 */
#define PROCESSORS_MANAGER Core::KernelManager<Core::ProcessorFactory, Base::KERNEL_PROCESSOR, KernelManagerAux::Processors>::instance()

#endif /* KERNELMANAGER_HPP_ */
