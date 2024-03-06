# Console application based on Shapes which demonstrates use of topic query 

Skeleton created via
 
`rtiddsgen -language C++11 -platform x64Linux4gcc7.3.0 -example x64Linux4gcc7.3.0 -create makefiles -create typefiles -d c++11 shapes.idl`

## Quality of Service
Changes to the Quality of Service were made to allow the DataWriter to respond to TopicQuery requests. This is the only one of three settings that prevents the functioning of TopicQueries by default.


```xml
<topic_query_dispatch>                    
    <enable>true</enable>                    
</topic_query_dispatch>
```

The other two QoS settings that control TopicQuery requests are:

1. By default, DDS_AsynchronousPublisherQosPolicy::disable_topic_query_publication is set to false. However, when this setting is set to true, no DataWriters can  be created in the publisher with DDS_TopicQueryDispatchQosPolicy::enable set to true.

```xml
<publisher_qos>                
    <asynchronous_publisher>
        <disable_topic_query_publication>false</disable_topic_query_publication>
    </asynchronous_publisher>
</publisher_qos>
```

2.  In order to send or receive TopicQuery requests, DomainParticipants must enable the service request channel. This channel is enabled by default. It is controlled by the  DDS_DiscoveryConfigQosPolicy::enabled_builtin_channels setting
                
```xml
<discovery_config>
    <enabled_builtin_channels>MASK_ALL</enabled_builtin_channels>
</discovery_config>
```


In addition to the TopicQuery, changes were made to ensure that the DataWriter's cache had some data to be queried.

```xml
 <history>
    <kind>KEEP_LAST_HISTORY_QOS</kind>
    <depth>50</depth>
</history>
<durability>
    <kind>TRANSIENT_LOCAL_DURABILITY_QOS</kind>
    <writer_depth>AUTO_WRITER_DEPTH</writer_depth>                    
</durability>
```


## Publisher

The publisher publishes an Orange shapetype to the Circle topic with a sine wave motion. The shapesize is set to 30. Running ShapesDemo will allow the shape to be seen. Contrast the output with that displayed by the subscriber application, which only displays those samples matching the topic filter every ten seconds.

## Subscriber

The subscriber has a content filter which filters out samples with a shapesize greater than 0. This ensures that no data is sent from the DataWriter to the DataReader via normal operation.  

In addition to the content filter, a topic filter is created which filters samples with y >= 0 and y <= 50.
Instead of the usual waitset inside a loop to capture data, the loop sleeps for a second and every ten seconds the process_data function is called. This loop continues until either the maximum samples are read, or ctrl+c is pressed.  

The process_data function `take`s the samples from the reader and displays the received samples in one of two ways.

1. For those samples which are `valid`, of which none should be received, the sample data is output to the console with the prefix "Live sample data:"

2. For those samples which are non `valid` and have a known topic_query_guid, the sample data is output to the console with the prefix "Queried sample data:".
