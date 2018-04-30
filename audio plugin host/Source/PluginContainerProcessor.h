/*
  ==============================================================================

    PluginContainerProcessor.h
    Created: 27 Apr 2018 10:53:23pm
    Author:  Agneya Kerure

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include <iostream>
#include <fstream>

class PluginContainerProcessor : public AudioProcessor,
                                 public ChangeBroadcaster,
                                 private AsyncUpdater
{
public:
    //==============================================================================
    /** Creates an empty graph. */
    PluginContainerProcessor();
    void generateAudioFile(bool rec);
    
    /** Destructor.
     Any processor objects that have been added to the graph will also be deleted.
     */
    ~PluginContainerProcessor();
    
    /** Each node in the graph has a UID of this type. */
    typedef uint32 NodeID;
    
    //==============================================================================
    /** A special index that represents the midi channel of a node.
     
     This is used as a channel index value if you want to refer to the midi input
     or output instead of an audio channel.
     */
    enum { midiChannelIndex = 0x1000 };
    
    //==============================================================================
    /**
     Represents an input or output channel of a node in an PluginContainerProcessor.
     */
    struct NodeAndChannel
    {
        NodeID nodeID;
        int channelIndex;
        
        bool isMIDI() const noexcept                                    { return channelIndex == midiChannelIndex; }
        
        bool operator== (const NodeAndChannel& other) const noexcept    { return nodeID == other.nodeID && channelIndex == other.channelIndex; }
        bool operator!= (const NodeAndChannel& other) const noexcept    { return ! operator== (other); }
    };
    
    //==============================================================================
    /** Represents one of the nodes, or processors, in an PluginContainerProcessor.
     
     To create a node, call PluginContainerProcessor::addNode().
     */
    class Node   : public ReferenceCountedObject
    {
    public:
        //==============================================================================
        /** The ID number assigned to this node.
         This is assigned by the graph that owns it, and can't be changed.
         */
        const NodeID nodeID;
        
        /** The actual processor object that this node represents. */
        AudioProcessor* getProcessor() const noexcept           { return processor.get(); }
        
        /** A set of user-definable properties that are associated with this node.
         
         This can be used to attach values to the node for whatever purpose seems
         useful. For example, you might store an x and y position if your application
         is displaying the nodes on-screen.
         */
        NamedValueSet properties;
        
        //==============================================================================
        /** A convenient typedef for referring to a pointer to a node object. */
        typedef ReferenceCountedObjectPtr<Node> Ptr;
        
    private:
        //==============================================================================
        friend class PluginContainerProcessor;
        
        struct Connection
        {
            Node* otherNode;
            int otherChannel, thisChannel;
            
            bool operator== (const Connection&) const noexcept;
        };
        
        const ScopedPointer<AudioProcessor> processor;
        Array<Connection> inputs, outputs;
        bool isPrepared = false;
        
        Node (NodeID, AudioProcessor*) noexcept;
        
        void setParentGraph (PluginContainerProcessor*) const;
        void prepare (double newSampleRate, int newBlockSize, PluginContainerProcessor*, ProcessingPrecision);
        void unprepare();
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Node)
    };
    
    //==============================================================================
    /** Represents a connection between two channels of two nodes in an PluginContainerProcessor.
     
     To create a connection, use PluginContainerProcessor::addConnection().
     */
    struct Connection
    {
        //==============================================================================
        Connection (NodeAndChannel source, NodeAndChannel destination) noexcept;
        
        Connection (const Connection&) = default;
        Connection& operator= (const Connection&) = default;
        
        bool operator== (const Connection&) const noexcept;
        bool operator!= (const Connection&) const noexcept;
        bool operator<  (const Connection&) const noexcept;
        
        //==============================================================================
        /** The channel and node which is the input source for this connection. */
        NodeAndChannel source;
        
        /** The channel and node which is the input source for this connection. */
        NodeAndChannel destination;
    };
    
    //==============================================================================
    /** Deletes all nodes and connections from this graph.
     Any processor objects in the graph will be deleted.
     */
    void clear();
    
    /** Returns the array of nodes in the graph. */
    const ReferenceCountedArray<Node>& getNodes() const noexcept    { return nodes; }
    
    /** Returns the number of nodes in the graph. */
    int getNumNodes() const noexcept                                { return nodes.size(); }
    
    /** Returns a pointer to one of the nodes in the graph.
     This will return nullptr if the index is out of range.
     @see getNodeForId
     */
    Node* getNode (int index) const noexcept                        { return nodes [index]; }
    
    /** Searches the graph for a node with the given ID number and returns it.
     If no such node was found, this returns nullptr.
     @see getNode
     */
    Node* getNodeForId (NodeID) const;
    
    /** Adds a node to the graph.
     
     This creates a new node in the graph, for the specified processor. Once you have
     added a processor to the graph, the graph owns it and will delete it later when
     it is no longer needed.
     
     The optional nodeId parameter lets you specify an ID to use for the node, but
     if the value is already in use, this new node will overwrite the old one.
     
     If this succeeds, it returns a pointer to the newly-created node.
     */
    Node::Ptr addNode (AudioProcessor* newProcessor, NodeID nodeId = {});
    
    /** Deletes a node within the graph which has the specified ID.
     This will also delete any connections that are attached to this node.
     */
    bool removeNode (NodeID);
    
    /** Deletes a node within the graph.
     This will also delete any connections that are attached to this node.
     */
    bool removeNode (Node*);
    
    /** Returns the list of connections in the graph. */
    std::vector<Connection> getConnections() const;
    
    /** Returns true if the given connection exists. */
    bool isConnected (const Connection&) const noexcept;
    
    /** Returns true if there is a direct connection between any of the channels of
     two specified nodes.
     */
    bool isConnected (NodeID possibleSourceNodeID, NodeID possibleDestNodeID) const noexcept;
    
    /** Does a recursive check to see if there's a direct or indirect series of connections
     between these two nodes.
     */
    bool isAnInputTo (Node& source, Node& destination) const noexcept;
    
    /** Returns true if it would be legal to connect the specified points. */
    bool canConnect (const Connection&) const;
    
    /** Attempts to connect two specified channels of two nodes.
     
     If this isn't allowed (e.g. because you're trying to connect a midi channel
     to an audio one or other such nonsense), then it'll return false.
     */
    bool addConnection (const Connection&);
    
    /** Deletes the given connection. */
    bool removeConnection (const Connection&);
    
    /** Removes all connections from the specified node. */
    bool disconnectNode (NodeID);
    
    /** Returns true if the given connection's channel numbers map on to valid
     channels at each end.
     Even if a connection is valid when created, its status could change if
     a node changes its channel config.
     */
    bool isConnectionLegal (const Connection&) const;
    
    /** Performs a sanity checks of all the connections.
     
     This might be useful if some of the processors are doing things like changing
     their channel counts, which could render some connections obsolete.
     */
    bool removeIllegalConnections();
    
    //==============================================================================
    /** A special type of AudioProcessor that can live inside an PluginContainerProcessor
     in order to use the audio that comes into and out of the graph itself.
     
     If you create an AudioGraphIOProcessor in "input" mode, it will act as a
     node in the graph which delivers the audio that is coming into the parent
     graph. This allows you to stream the data to other nodes and process the
     incoming audio.
     
     Likewise, one of these in "output" mode can be sent data which it will add to
     the sum of data being sent to the graph's output.
     
     @see PluginContainerProcessor
     */
    class JUCE_API  AudioGraphIOProcessor     : public AudioPluginInstance
    {
    public:
        /** Specifies the mode in which this processor will operate.
         */
        enum IODeviceType
        {
            audioInputNode,     /**< In this mode, the processor has output channels
                                 representing all the audio input channels that are
                                 coming into its parent audio graph. */
            audioOutputNode,    /**< In this mode, the processor has input channels
                                 representing all the audio output channels that are
                                 going out of its parent audio graph. */
            midiInputNode,      /**< In this mode, the processor has a midi output which
                                 delivers the same midi data that is arriving at its
                                 parent graph. */
            midiOutputNode      /**< In this mode, the processor has a midi input and
                                 any data sent to it will be passed out of the parent
                                 graph. */
        };
        
        //==============================================================================
        /** Returns the mode of this processor. */
        IODeviceType getType() const noexcept                       { return type; }
        
        /** Returns the parent graph to which this processor belongs, or nullptr if it
         hasn't yet been added to one. */
        PluginContainerProcessor* getParentGraph() const noexcept        { return graph; }
        
        /** True if this is an audio or midi input. */
        bool isInput() const noexcept;
        /** True if this is an audio or midi output. */
        bool isOutput() const noexcept;
        
        //==============================================================================
        AudioGraphIOProcessor (IODeviceType);
        ~AudioGraphIOProcessor();
        
        const String getName() const override;
        void fillInPluginDescription (PluginDescription&) const override;
        void prepareToPlay (double newSampleRate, int estimatedSamplesPerBlock) override;
        void releaseResources() override;
        void processBlock (AudioBuffer<float>& , MidiBuffer&) override;
        void processBlock (AudioBuffer<double>&, MidiBuffer&) override;
        bool supportsDoublePrecisionProcessing() const override;
        
        double getTailLengthSeconds() const override;
        bool acceptsMidi() const override;
        bool producesMidi() const override;
        
        bool hasEditor() const override;
        AudioProcessorEditor* createEditor() override;
        
        int getNumPrograms() override;
        int getCurrentProgram() override;
        void setCurrentProgram (int) override;
        const String getProgramName (int) override;
        void changeProgramName (int, const String&) override;
        
        void getStateInformation (juce::MemoryBlock& destData) override;
        void setStateInformation (const void* data, int sizeInBytes) override;
        
        /** @internal */
        void setParentGraph (PluginContainerProcessor*);
        
    private:
        const IODeviceType type;
        PluginContainerProcessor* graph = nullptr;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioGraphIOProcessor)
    };
    
    //==============================================================================
    const String getName() const override;
    void prepareToPlay (double, int) override;
    void releaseResources() override;
    void processBlock (AudioBuffer<float>&,  MidiBuffer&) override;
    void processBlock (AudioBuffer<double>&, MidiBuffer&) override;
    bool supportsDoublePrecisionProcessing() const override;
    
    void reset() override;
    void setNonRealtime (bool) noexcept override;
    
    double getTailLengthSeconds() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    
    bool hasEditor() const override                         { return false; }
    AudioProcessorEditor* createEditor() override           { return nullptr; }
    int getNumPrograms() override                           { return 0; }
    int getCurrentProgram() override                        { return 0; }
    void setCurrentProgram (int) override                   { }
    const String getProgramName (int) override              { return {}; }
    void changeProgramName (int, const String&) override    { }
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
private:
    //==============================================================================
    ReferenceCountedArray<Node> nodes;
    NodeID lastNodeID = {};
    
    struct RenderSequenceFloat;
    struct RenderSequenceDouble;
    ScopedPointer<RenderSequenceFloat> renderSequenceFloat;
    ScopedPointer<RenderSequenceDouble> renderSequenceDouble;
    
    friend class AudioGraphIOProcessor;
    
    bool isPrepared = false;
    
    void topologyChanged();
    void handleAsyncUpdate() override;
    void clearRenderingSequence();
    void buildRenderingSequence();
    bool anyNodesNeedPreparing() const noexcept;
    bool isConnected (Node* src, int sourceChannel, Node* dest, int destChannel) const noexcept;
    bool isAnInputTo (Node& src, Node& dst, int recursionCheck) const noexcept;
    bool canConnect (Node* src, int sourceChannel, Node* dest, int destChannel) const noexcept;
    bool isLegal (Node* src, int sourceChannel, Node* dest, int destChannel) const noexcept;
    static void getNodeConnections (Node&, std::vector<Connection>&);
    
    
    
//    ScopedPointer<AudioSampleBuffer> m_asbWriteBuffer;
//    ScopedPointer<WavAudioFormat> m_wWriteFormat;
//    File m_fOutputFile;
//    ScopedPointer<FileOutputStream> m_OutputTo;
//    ScopedPointer<AudioFormatWriter> m_AudioFormatWriter;
//    ScopedPointer<AudioFormatWriter::ThreadedWriter> threaded;
    
    std::string m_sOutputFilePath;
    
    bool m_bRecording;
    float** m_ppfStorageBuffer;
    int m_iLastLoc;
    
    std::ofstream m_fMyFile;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginContainerProcessor)

};