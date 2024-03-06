/*
* (c) Copyright, Real-Time Innovations, 2020.  All rights reserved.
* RTI grants Licensee a license to use, modify, compile, and create derivative
* works of the software solely for use with RTI Connext DDS. Licensee may
* redistribute copies of the software provided that all such copies are subject
* to this license. The software is provided "as is", with no warranty of any
* type, including any warranty for fitness for any purpose. RTI is under no
* obligation to maintain or support the software. RTI shall not be liable for
* any incidental or consequential damages arising out of the use or inability
* to use the software.
*/

#include <algorithm>
#include <iostream>
#include <chrono>
#include <sstream>

#include <dds/sub/ddssub.hpp>
#include <rti/sub/TopicQuery.hpp>
#include <rti/core/Guid.hpp>
#include <dds/core/ddscore.hpp>
#include <rti/config/Logger.hpp>  // for logging
#include <rti/util/util.hpp>      // for sleep()

// alternatively, to include all the standard APIs:
//  <dds/dds.hpp>
// or to include both the standard APIs and extensions:
//  <rti/rti.hpp>
//
// For more information about the headers and namespaces, see:
//    https://community.rti.com/static/documentation/connext-dds/7.2.0/doc/api/connext_dds/api_cpp2/group__DDSNamespaceModule.html
// For information on how to use extensions, see:
//    https://community.rti.com/static/documentation/connext-dds/7.2.0/doc/api/connext_dds/api_cpp2/group__DDSCpp2Conventions.html

#include "shapes.hpp"
#include "application.hpp"  // for command line parsing and ctrl-c
#include <string>


using std::cout;
using std::chrono::system_clock;
using std::chrono::duration_cast;
using std::chrono::seconds;
using std::to_string;

static const char endl = '\n';

void process_data(dds::sub::DataReader< ::ShapeTypeExtended> reader, unsigned int& sample_count)
{    
    // Take all samples
    
    dds::sub::LoanedSamples< ::ShapeTypeExtended> samples = reader.take();
    for (auto sample : samples) {
        if (sample.info().valid()) {
            ++sample_count;
            if (sample.info().extensions().topic_query_guid() != rti::core::Guid::unknown())
                cout << "Queried sample data: " << sample.data() << endl; 
            else
                cout << "Live sample data: " << sample.data() << endl;
        } 
        else {
            cout << "Instance state changed to "
                << sample.info().state().instance_state() << endl;
        }
    }
} // The LoanedSamples destructor returns the loan

void run_subscriber_application(unsigned int domain_id, unsigned int sample_count)
{
    // DDS objects behave like shared pointers or value types
    // (see https://community.rti.com/best-practices/use-modern-c-types-correctly)

    // Start communicating in a domain, usually one participant per application
    dds::domain::DomainParticipant participant(domain_id);

    // Create a Topic with a name and a datatype
    dds::topic::Topic< ::ShapeTypeExtended> topic(participant, "Circle");

    // Create a Subscriber and DataReader with default Qos
    dds::sub::Subscriber subscriber(participant);    
        
    // Define a content filter to essentially disable the datawriter sending anything on the wire
    auto filtered_topic = dds::topic::ContentFilteredTopic< ::ShapeTypeExtended>(
                topic,
                "ContentFilteredTopic",
                dds::topic::Filter("shapesize < %0", { to_string(0) }));

    dds::sub::DataReader< ::ShapeTypeExtended> reader(subscriber, filtered_topic);

    // Define a topic filter
    int y_min = 0, y_max = 50;
    auto filter = dds::topic::Filter("y >= %0 AND y <= %1", { to_string(y_min), to_string(y_max) });

    // Set the Selection kind (limited to cached samples or not)
    auto kind = rti::sub::TopicQuerySelectionKind::CONTINUOUS;
    
    // define a TopicQuery Selection from the filter
    auto topic_selection = rti::sub::TopicQuerySelection(filter, kind);

    // Create a TopicQuery
    auto topic_query = rti::sub::TopicQuery(reader, topic_selection);

    cout << "Create a topic query with GUID: " << topic_query.guid()
          << ", filtering with y >= " << y_min << " and y <= " << y_max << endl;
    
    unsigned int samples_read = 0;
    auto then = system_clock::now(), now = then;
    while (!application::shutdown_requested && samples_read < sample_count) {

        now = system_clock::now();
        if (duration_cast<seconds>(now - then).count() >= 10) { 
            then = now;

            cout << "Checking for data via topic query" << endl;
            process_data(reader, sample_count);
        }
        rti::util::sleep(dds::core::Duration(1));
    }
      
    // Topic filter is cleaned up when all references go out of scope
}

int main(int argc, char *argv[])
{

    using namespace application;

    // Parse arguments and handle control-C
    auto arguments = parse_arguments(argc, argv);
    if (arguments.parse_result == ParseReturn::exit) {
        return EXIT_SUCCESS;
    } else if (arguments.parse_result == ParseReturn::failure) {
        return EXIT_FAILURE;
    }
    setup_signal_handlers();

    // Sets Connext verbosity to help debugging
    rti::config::Logger::instance().verbosity(arguments.verbosity);

    try {
        run_subscriber_application(arguments.domain_id, arguments.sample_count);
    } catch (const std::exception& ex) {
        // This will catch DDS exceptions
        std::cerr << "Exception in run_subscriber_application(): " << ex.what()
        << endl;
        return EXIT_FAILURE;
    }

    // Releases the memory used by the participant factory.  Optional at
    // application exit
    dds::domain::DomainParticipant::finalize_participant_factory();

    return EXIT_SUCCESS;
}
