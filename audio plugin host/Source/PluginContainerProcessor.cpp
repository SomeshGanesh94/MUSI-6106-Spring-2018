/*
  ==============================================================================

    PluginContainerProcessor.cpp
    Created: 27 Apr 2018 10:53:23pm
    Author:  Agneya Kerure

  ==============================================================================
*/

#include "PluginContainerProcessor.h"

using namespace std;

#include <iostream>
#include <fstream>

template <typename FloatType>
struct GraphRenderSequence
{
    GraphRenderSequence() {}
    
    struct Context
    {
        FloatType** audioBuffers;
        MidiBuffer* midiBuffers;
        AudioPlayHead* audioPlayHead;
        int numSamples;
    };
    
    void perform (AudioBuffer<FloatType>& buffer, MidiBuffer& midiMessages, AudioPlayHead* audioPlayHead)
    {
        auto numSamples = buffer.getNumSamples();
        auto maxSamples = renderingBuffer.getNumSamples();
        
        if (numSamples > maxSamples)
        {
            // being asked to render more samples than our buffers have, so slice things up...
            tempMIDI.clear();
            tempMIDI.addEvents (midiMessages, maxSamples, numSamples, -maxSamples);
            
            {
                AudioBuffer<FloatType> startAudio (buffer.getArrayOfWritePointers(), buffer.getNumChannels(), maxSamples);
                midiMessages.clear (maxSamples, numSamples);
                perform (startAudio, midiMessages, audioPlayHead);
            }
            
            AudioBuffer<FloatType> endAudio (buffer.getArrayOfWritePointers(), buffer.getNumChannels(), maxSamples, numSamples - maxSamples);
            perform (endAudio, tempMIDI, audioPlayHead);
            return;
        }
        
        currentAudioInputBuffer = &buffer;
        currentAudioOutputBuffer.setSize (jmax (1, buffer.getNumChannels()), numSamples);
        currentAudioOutputBuffer.clear();
        currentMidiInputBuffer = &midiMessages;
        currentMidiOutputBuffer.clear();
        
        {
            const Context context { renderingBuffer.getArrayOfWritePointers(), midiBuffers.begin(), audioPlayHead, numSamples };
            
            for (auto* op : renderOps)
                op->perform (context);
        }
        
        for (int i = 0; i < buffer.getNumChannels(); ++i)
            buffer.copyFrom (i, 0, currentAudioOutputBuffer, i, 0, numSamples);
        
        midiMessages.clear();
        midiMessages.addEvents (currentMidiOutputBuffer, 0, buffer.getNumSamples(), 0);
        currentAudioInputBuffer = nullptr;
    }
    
    void addClearChannelOp (int index)
    {
        createOp ([=] (const Context& c)    { FloatVectorOperations::clear (c.audioBuffers[index], c.numSamples); });
    }
    
    void addCopyChannelOp (int srcIndex, int dstIndex)
    {
        createOp ([=] (const Context& c)    { FloatVectorOperations::copy (c.audioBuffers[dstIndex],
                                                                           c.audioBuffers[srcIndex],
                                                                           c.numSamples); });
    }
    
    void addAddChannelOp (int srcIndex, int dstIndex)
    {
        createOp ([=] (const Context& c)    { FloatVectorOperations::add (c.audioBuffers[dstIndex],
                                                                          c.audioBuffers[srcIndex],
                                                                          c.numSamples); });
    }
    
    void addClearMidiBufferOp (int index)
    {
        createOp ([=] (const Context& c)    { c.midiBuffers[index].clear(); });
    }
    
    void addCopyMidiBufferOp (int srcIndex, int dstIndex)
    {
        createOp ([=] (const Context& c)    { c.midiBuffers[dstIndex] = c.midiBuffers[srcIndex]; });
    }
    
    void addAddMidiBufferOp (int srcIndex, int dstIndex)
    {
        createOp ([=] (const Context& c)    { c.midiBuffers[dstIndex].addEvents (c.midiBuffers[srcIndex],
                                                                                 0, c.numSamples, 0); });
    }
    
    void addDelayChannelOp (int chan, int delaySize)
    {
        renderOps.add (new DelayChannelOp (chan, delaySize));
    }
    
    void addProcessOp (const PluginContainerProcessor::Node::Ptr& node,
                       const Array<int>& audioChannelsUsed, int totalNumChans, int midiBuffer)
    {
        renderOps.add (new ProcessOp (node, audioChannelsUsed, totalNumChans, midiBuffer));
    }
    
    void prepareBuffers (int blockSize)
    {
        renderingBuffer.setSize (numBuffersNeeded + 1, blockSize);
        renderingBuffer.clear();
        currentAudioOutputBuffer.setSize (numBuffersNeeded + 1, blockSize);
        currentAudioOutputBuffer.clear();
        
        currentAudioInputBuffer = nullptr;
        currentMidiInputBuffer = nullptr;
        currentMidiOutputBuffer.clear();
        
        midiBuffers.clearQuick();
        midiBuffers.resize (numMidiBuffersNeeded);
        
        const int defaultMIDIBufferSize = 512;
        
        tempMIDI.ensureSize (defaultMIDIBufferSize);
        
        for (auto&& m : midiBuffers)
            m.ensureSize (defaultMIDIBufferSize);
    }
    
    void releaseBuffers()
    {
        renderingBuffer.setSize (1, 1);
        currentAudioOutputBuffer.setSize (1, 1);
        currentAudioInputBuffer = nullptr;
        currentMidiInputBuffer = nullptr;
        currentMidiOutputBuffer.clear();
        midiBuffers.clear();
    }
    
    int numBuffersNeeded = 0, numMidiBuffersNeeded = 0;
    
    AudioBuffer<FloatType> renderingBuffer, currentAudioOutputBuffer;
    AudioBuffer<FloatType>* currentAudioInputBuffer = nullptr;
    
    MidiBuffer* currentMidiInputBuffer = nullptr;
    MidiBuffer currentMidiOutputBuffer;
    
    Array<MidiBuffer> midiBuffers;
    MidiBuffer tempMIDI;
    
private:
    //==============================================================================
    struct RenderingOp
    {
        RenderingOp() noexcept {}
        virtual ~RenderingOp() {}
        virtual void perform (const Context&) = 0;
        
        JUCE_LEAK_DETECTOR (RenderingOp)
    };
    
    OwnedArray<RenderingOp> renderOps;
    
    //==============================================================================
    template <typename LambdaType>
    void createOp (LambdaType&& fn)
    {
        struct LambdaOp  : public RenderingOp
        {
            LambdaOp (LambdaType&& f) : function (static_cast<LambdaType&&> (f)) {}
            void perform (const Context& c) override    { function (c); }
            
            LambdaType function;
        };
        
        renderOps.add (new LambdaOp (static_cast<LambdaType&&> (fn)));
    }
    
    //==============================================================================
    struct DelayChannelOp  : public RenderingOp
    {
        DelayChannelOp (int chan, int delaySize)
        : channel (chan),
        bufferSize (delaySize + 1),
        writeIndex (delaySize)
        {
            buffer.calloc ((size_t) bufferSize);
        }
        
        void perform (const Context& c) override
        {
            auto* data = c.audioBuffers[channel];
            
            for (int i = c.numSamples; --i >= 0;)
            {
                buffer[writeIndex] = *data;
                *data++ = buffer[readIndex];
                
                if (++readIndex  >= bufferSize) readIndex = 0;
                if (++writeIndex >= bufferSize) writeIndex = 0;
            }
        }
        
        HeapBlock<FloatType> buffer;
        const int channel, bufferSize;
        int readIndex = 0, writeIndex;
        
        JUCE_DECLARE_NON_COPYABLE (DelayChannelOp)
    };
    
    //==============================================================================
    struct ProcessOp   : public RenderingOp
    {
        ProcessOp (const PluginContainerProcessor::Node::Ptr& n,
                   const Array<int>& audioChannelsUsed,
                   int totalNumChans, int midiBuffer)
        : node (n),
        processor (*n->getProcessor()),
        audioChannelsToUse (audioChannelsUsed),
        totalChans (jmax (1, totalNumChans)),
        midiBufferToUse (midiBuffer)
        {
            audioChannels.calloc ((size_t) totalChans);
            
            while (audioChannelsToUse.size() < totalChans)
                audioChannelsToUse.add (0);
        }
        
        void perform (const Context& c) override
        {
            processor.setPlayHead (c.audioPlayHead);
            
            for (int i = 0; i < totalChans; ++i)
                audioChannels[i] = c.audioBuffers[audioChannelsToUse.getUnchecked (i)];
            
            AudioBuffer<FloatType> buffer (audioChannels, totalChans, c.numSamples);
            
            if (processor.isSuspended())
                buffer.clear();
            else
                callProcess (buffer, c.midiBuffers[midiBufferToUse]);
        }
        
        void callProcess (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
        {
            if (processor.isUsingDoublePrecision())
            {
                tempBufferDouble.makeCopyOf (buffer, true);
                processor.processBlock (tempBufferDouble, midiMessages);
                buffer.makeCopyOf (tempBufferDouble, true);
            }
            else
            {
                processor.processBlock (buffer, midiMessages);
            }
        }
        
        void callProcess (AudioBuffer<double>& buffer, MidiBuffer& midiMessages)
        {
            if (processor.isUsingDoublePrecision())
            {
                processor.processBlock (buffer, midiMessages);
            }
            else
            {
                tempBufferFloat.makeCopyOf (buffer, true);
                processor.processBlock (tempBufferFloat, midiMessages);
                buffer.makeCopyOf (tempBufferFloat, true);
            }
        }
        
        const PluginContainerProcessor::Node::Ptr node;
        AudioProcessor& processor;
        
        Array<int> audioChannelsToUse;
        HeapBlock<FloatType*> audioChannels;
        AudioBuffer<float> tempBufferFloat, tempBufferDouble;
        const int totalChans, midiBufferToUse;
        
        JUCE_DECLARE_NON_COPYABLE (ProcessOp)
    };
};

//==============================================================================
//==============================================================================
template <typename RenderSequence>
struct RenderSequenceBuilder
{
    RenderSequenceBuilder (PluginContainerProcessor& g, RenderSequence& s)
    : graph (g), sequence (s)
    {
        createOrderedNodeList();
        
        audioBuffers.add (AssignedBuffer::createReadOnlyEmpty()); // first buffer is read-only zeros
        midiBuffers .add (AssignedBuffer::createReadOnlyEmpty());
        
        for (int i = 0; i < orderedNodes.size(); ++i)
        {
            createRenderingOpsForNode (*orderedNodes.getUnchecked(i), i);
            markAnyUnusedBuffersAsFree (audioBuffers, i);
            markAnyUnusedBuffersAsFree (midiBuffers, i);
        }
        
        graph.setLatencySamples (totalLatency);
        
        s.numBuffersNeeded = audioBuffers.size();
        s.numMidiBuffersNeeded = midiBuffers.size();
    }
    
    //==============================================================================
    typedef PluginContainerProcessor::NodeID NodeID;
    
    PluginContainerProcessor& graph;
    RenderSequence& sequence;
    
    Array<PluginContainerProcessor::Node*> orderedNodes;
    
    struct AssignedBuffer
    {
        PluginContainerProcessor::NodeAndChannel channel;
        
        static AssignedBuffer createReadOnlyEmpty() noexcept    { return { { (NodeID) zeroNodeID, 0 } }; }
        static AssignedBuffer createFree() noexcept             { return { { (NodeID) freeNodeID, 0 } }; }
        
        bool isReadOnlyEmpty() const noexcept                   { return channel.nodeID == (NodeID) zeroNodeID; }
        bool isFree() const noexcept                            { return channel.nodeID == (NodeID) freeNodeID; }
        bool isAssigned() const noexcept                        { return ! (isReadOnlyEmpty() || isFree()); }
        
        void setFree() noexcept                                 { channel = { (NodeID) freeNodeID, 0 }; }
        void setAssignedToNonExistentNode() noexcept            { channel = { (NodeID) anonNodeID, 0 }; }
        
    private:
        enum
        {
            anonNodeID = 0x7ffffffd,
            zeroNodeID = 0x7ffffffe,
            freeNodeID = 0x7fffffff
        };
    };
    
    Array<AssignedBuffer> audioBuffers, midiBuffers;
    
    enum { readOnlyEmptyBufferIndex = 0 };
    
    struct Delay
    {
        NodeID nodeID;
        int delay;
    };
    
    HashMap<NodeID, int> delays;
    int totalLatency = 0;
    
    int getNodeDelay (NodeID nodeID) const noexcept
    {
        return delays[nodeID];
    }
    
    int getInputLatencyForNode (NodeID nodeID) const
    {
        int maxLatency = 0;
        
        for (auto&& c : graph.getConnections())
            if (c.destination.nodeID == nodeID)
                maxLatency = jmax (maxLatency, getNodeDelay (c.source.nodeID));
        
        return maxLatency;
    }
    
    //==============================================================================
    void createOrderedNodeList()
    {
        for (auto* node : graph.getNodes())
        {
            int j = 0;
            
            for (; j < orderedNodes.size(); ++j)
                if (graph.isAnInputTo (*node, *orderedNodes.getUnchecked(j)))
                    break;
            
            orderedNodes.insert (j, node);
        }
    }
    
    int findBufferForInputAudioChannel (PluginContainerProcessor::Node& node, const int inputChan,
                                        const int ourRenderingIndex, const int maxLatency)
    {
        auto& processor = *node.getProcessor();
        auto numOuts = processor.getTotalNumOutputChannels();
        
        auto sources = getSourcesForChannel (node, inputChan);
        
        // Handle an unconnected input channel...
        if (sources.isEmpty())
        {
            if (inputChan >= numOuts)
                return readOnlyEmptyBufferIndex;
            
            auto index = getFreeBuffer (audioBuffers);
            sequence.addClearChannelOp (index);
            return index;
        }
        
        // Handle an input from a single source..
        if (sources.size() == 1)
        {
            // channel with a straightforward single input..
            auto src = sources.getUnchecked(0);
            
            int bufIndex = getBufferContaining (src);
            
            if (bufIndex < 0)
            {
                // if not found, this is probably a feedback loop
                bufIndex = readOnlyEmptyBufferIndex;
                jassert (bufIndex >= 0);
            }
            
            if (inputChan < numOuts
                && isBufferNeededLater (ourRenderingIndex, inputChan, src))
            {
                // can't mess up this channel because it's needed later by another node,
                // so we need to use a copy of it..
                auto newFreeBuffer = getFreeBuffer (audioBuffers);
                sequence.addCopyChannelOp (bufIndex, newFreeBuffer);
                bufIndex = newFreeBuffer;
            }
            
            auto nodeDelay = getNodeDelay (src.nodeID);
            
            if (nodeDelay < maxLatency)
                sequence.addDelayChannelOp (bufIndex, maxLatency - nodeDelay);
            
            return bufIndex;
        }
        
        // Handle a mix of several outputs coming into this input..
        int reusableInputIndex = -1;
        int bufIndex = -1;
        
        for (int i = 0; i < sources.size(); ++i)
        {
            auto src = sources.getReference(i);
            auto sourceBufIndex = getBufferContaining (src);
            
            if (sourceBufIndex >= 0 && ! isBufferNeededLater (ourRenderingIndex, inputChan, src))
            {
                // we've found one of our input chans that can be re-used..
                reusableInputIndex = i;
                bufIndex = sourceBufIndex;
                
                auto nodeDelay = getNodeDelay (src.nodeID);
                
                if (nodeDelay < maxLatency)
                    sequence.addDelayChannelOp (bufIndex, maxLatency - nodeDelay);
                
                break;
            }
        }
        
        if (reusableInputIndex < 0)
        {
            // can't re-use any of our input chans, so get a new one and copy everything into it..
            bufIndex = getFreeBuffer (audioBuffers);
            jassert (bufIndex != 0);
            
            audioBuffers.getReference (bufIndex).setAssignedToNonExistentNode();
            
            auto srcIndex = getBufferContaining (sources.getFirst());
            
            if (srcIndex < 0)
                sequence.addClearChannelOp (bufIndex);  // if not found, this is probably a feedback loop
            else
                sequence.addCopyChannelOp (srcIndex, bufIndex);
            
            reusableInputIndex = 0;
            auto nodeDelay = getNodeDelay (sources.getFirst().nodeID);
            
            if (nodeDelay < maxLatency)
                sequence.addDelayChannelOp (bufIndex, maxLatency - nodeDelay);
        }
        
        for (int i = 0; i < sources.size(); ++i)
        {
            if (i != reusableInputIndex)
            {
                auto src = sources.getReference(i);
                int srcIndex = getBufferContaining (src);
                
                if (srcIndex >= 0)
                {
                    auto nodeDelay = getNodeDelay (src.nodeID);
                    
                    if (nodeDelay < maxLatency)
                    {
                        if (! isBufferNeededLater (ourRenderingIndex, inputChan, src))
                        {
                            sequence.addDelayChannelOp (srcIndex, maxLatency - nodeDelay);
                        }
                        else // buffer is reused elsewhere, can't be delayed
                        {
                            auto bufferToDelay = getFreeBuffer (audioBuffers);
                            sequence.addCopyChannelOp (srcIndex, bufferToDelay);
                            sequence.addDelayChannelOp (bufferToDelay, maxLatency - nodeDelay);
                            srcIndex = bufferToDelay;
                        }
                    }
                    
                    sequence.addAddChannelOp (srcIndex, bufIndex);
                }
            }
        }
        
        return bufIndex;
    }
    
    int findBufferForInputMidiChannel (PluginContainerProcessor::Node& node, int ourRenderingIndex)
    {
        auto& processor = *node.getProcessor();
        auto sources = getSourcesForChannel (node, PluginContainerProcessor::midiChannelIndex);
        
        // No midi inputs..
        if (sources.isEmpty())
        {
            auto midiBufferToUse = getFreeBuffer (midiBuffers); // need to pick a buffer even if the processor doesn't use midi
            
            if (processor.acceptsMidi() || processor.producesMidi())
                sequence.addClearMidiBufferOp (midiBufferToUse);
            
            return midiBufferToUse;
        }
        
        // One midi input..
        if (sources.size() == 1)
        {
            auto src = sources.getReference (0);
            auto midiBufferToUse = getBufferContaining (src);
            
            if (midiBufferToUse >= 0)
            {
                if (isBufferNeededLater (ourRenderingIndex, PluginContainerProcessor::midiChannelIndex, src))
                {
                    // can't mess up this channel because it's needed later by another node, so we
                    // need to use a copy of it..
                    auto newFreeBuffer = getFreeBuffer (midiBuffers);
                    sequence.addCopyMidiBufferOp (midiBufferToUse, newFreeBuffer);
                    midiBufferToUse = newFreeBuffer;
                }
            }
            else
            {
                // probably a feedback loop, so just use an empty one..
                midiBufferToUse = getFreeBuffer (midiBuffers); // need to pick a buffer even if the processor doesn't use midi
            }
            
            return midiBufferToUse;
        }
        
        // Multiple midi inputs..
        int midiBufferToUse = -1;
        int reusableInputIndex = -1;
        
        for (int i = 0; i < sources.size(); ++i)
        {
            auto src = sources.getReference (i);
            auto sourceBufIndex = getBufferContaining (src);
            
            if (sourceBufIndex >= 0
                && ! isBufferNeededLater (ourRenderingIndex, PluginContainerProcessor::midiChannelIndex, src))
            {
                // we've found one of our input buffers that can be re-used..
                reusableInputIndex = i;
                midiBufferToUse = sourceBufIndex;
                break;
            }
        }
        
        if (reusableInputIndex < 0)
        {
            // can't re-use any of our input buffers, so get a new one and copy everything into it..
            midiBufferToUse = getFreeBuffer (midiBuffers);
            jassert (midiBufferToUse >= 0);
            
            auto srcIndex = getBufferContaining (sources.getUnchecked(0));
            
            if (srcIndex >= 0)
                sequence.addCopyMidiBufferOp (srcIndex, midiBufferToUse);
            else
                sequence.addClearMidiBufferOp (midiBufferToUse);
            
            reusableInputIndex = 0;
        }
        
        for (int i = 0; i < sources.size(); ++i)
        {
            if (i != reusableInputIndex)
            {
                auto srcIndex = getBufferContaining (sources.getUnchecked(i));
                
                if (srcIndex >= 0)
                    sequence.addAddMidiBufferOp (srcIndex, midiBufferToUse);
            }
        }
        
        return midiBufferToUse;
    }
    
    void createRenderingOpsForNode (PluginContainerProcessor::Node& node, const int ourRenderingIndex)
    {
        auto& processor = *node.getProcessor();
        auto numIns  = processor.getTotalNumInputChannels();
        auto numOuts = processor.getTotalNumOutputChannels();
        auto totalChans = jmax (numIns, numOuts);
        
        Array<int> audioChannelsToUse;
        auto maxLatency = getInputLatencyForNode (node.nodeID);
        
        for (int inputChan = 0; inputChan < numIns; ++inputChan)
        {
            // get a list of all the inputs to this node
            auto index = findBufferForInputAudioChannel (node, inputChan, ourRenderingIndex, maxLatency);
            jassert (index >= 0);
            
            audioChannelsToUse.add (index);
            
            if (inputChan < numOuts)
                audioBuffers.getReference (index).channel = { node.nodeID, inputChan };
        }
        
        for (int outputChan = numIns; outputChan < numOuts; ++outputChan)
        {
            auto index = getFreeBuffer (audioBuffers);
            jassert (index != 0);
            audioChannelsToUse.add (index);
            
            audioBuffers.getReference (index).channel = { node.nodeID, outputChan };
        }
        
        auto midiBufferToUse = findBufferForInputMidiChannel (node, ourRenderingIndex);
        
        if (processor.producesMidi())
            midiBuffers.getReference (midiBufferToUse).channel = { node.nodeID, PluginContainerProcessor::midiChannelIndex };
        
        delays.set (node.nodeID, maxLatency + processor.getLatencySamples());
        
        if (numOuts == 0)
            totalLatency = maxLatency;
        
        sequence.addProcessOp (&node, audioChannelsToUse, totalChans, midiBufferToUse);
    }
    
    //==============================================================================
    Array<PluginContainerProcessor::NodeAndChannel> getSourcesForChannel (PluginContainerProcessor::Node& node, int inputChannelIndex)
    {
        Array<PluginContainerProcessor::NodeAndChannel> results;
        PluginContainerProcessor::NodeAndChannel nc { node.nodeID, inputChannelIndex };
        
        for (auto&& c : graph.getConnections())
            if (c.destination == nc)
                results.add (c.source);
        
        return results;
    }
    
    static int getFreeBuffer (Array<AssignedBuffer>& buffers)
    {
        for (int i = 1; i < buffers.size(); ++i)
            if (buffers.getReference(i).isFree())
                return i;
        
        buffers.add (AssignedBuffer::createFree());
        return buffers.size() - 1;
    }
    
    int getBufferContaining (PluginContainerProcessor::NodeAndChannel output) const noexcept
    {
        int i = 0;
        
        for (auto& b : output.isMIDI() ? midiBuffers : audioBuffers)
        {
            if (b.channel == output)
                return i;
            
            ++i;
        }
        
        return -1;
    }
    
    void markAnyUnusedBuffersAsFree (Array<AssignedBuffer>& buffers, const int stepIndex)
    {
        for (auto& b : buffers)
            if (b.isAssigned() && ! isBufferNeededLater (stepIndex, -1, b.channel))
                b.setFree();
    }
    
    bool isBufferNeededLater (int stepIndexToSearchFrom,
                              int inputChannelOfIndexToIgnore,
                              PluginContainerProcessor::NodeAndChannel output) const
    {
        while (stepIndexToSearchFrom < orderedNodes.size())
        {
            auto* node = orderedNodes.getUnchecked (stepIndexToSearchFrom);
            
            if (output.isMIDI())
            {
                if (inputChannelOfIndexToIgnore != PluginContainerProcessor::midiChannelIndex
                    && graph.isConnected ({ { output.nodeID, PluginContainerProcessor::midiChannelIndex },
                    { node->nodeID,  PluginContainerProcessor::midiChannelIndex } }))
                    return true;
            }
            else
            {
                for (int i = 0; i < node->getProcessor()->getTotalNumInputChannels(); ++i)
                    if (i != inputChannelOfIndexToIgnore && graph.isConnected ({ output, { node->nodeID, i } }))
                        return true;
            }
            
            inputChannelOfIndexToIgnore = -1;
            ++stepIndexToSearchFrom;
        }
        
        return false;
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RenderSequenceBuilder)
};

//==============================================================================
PluginContainerProcessor::Connection::Connection (NodeAndChannel src, NodeAndChannel dst) noexcept
: source (src), destination (dst)
{
}

bool PluginContainerProcessor::Connection::operator== (const Connection& other) const noexcept
{
    return source == other.source && destination == other.destination;
}

bool PluginContainerProcessor::Connection::operator!= (const Connection& c) const noexcept
{
    return ! operator== (c);
}

bool PluginContainerProcessor::Connection::operator< (const Connection& other) const noexcept
{
    if (source.nodeID != other.source.nodeID)
        return source.nodeID < other.source.nodeID;
        
        if (destination.nodeID != other.destination.nodeID)
        return destination.nodeID < other.destination.nodeID;
        
        if (source.channelIndex != other.source.channelIndex)
        return source.channelIndex < other.source.channelIndex;
        
        return destination.channelIndex < other.destination.channelIndex;
        }
        
        //==============================================================================
        PluginContainerProcessor::Node::Node (NodeID n, AudioProcessor* p) noexcept
        : nodeID (n), processor (p)
    {
        jassert (processor != nullptr);
    }
        
        void PluginContainerProcessor::Node::prepare (double newSampleRate, int newBlockSize,
                                                 PluginContainerProcessor* graph, ProcessingPrecision precision)
    {
        if (! isPrepared)
        {
            isPrepared = true;
            setParentGraph (graph);
            
            // try to align the precision of the processor and the graph
            processor->setProcessingPrecision (processor->supportsDoublePrecisionProcessing() ? precision
                                               : singlePrecision);
            
            processor->setRateAndBufferSizeDetails (newSampleRate, newBlockSize);
            processor->prepareToPlay (newSampleRate, newBlockSize);
        }
    }
        
        void PluginContainerProcessor::Node::unprepare()
    {
        if (isPrepared)
        {
            isPrepared = false;
            processor->releaseResources();
        }
    }
        
        void PluginContainerProcessor::Node::setParentGraph (PluginContainerProcessor* const graph) const
    {
        if (auto* ioProc = dynamic_cast<PluginContainerProcessor::AudioGraphIOProcessor*> (processor.get()))
            ioProc->setParentGraph (graph);
    }
        
        bool PluginContainerProcessor::Node::Connection::operator== (const Connection& other) const noexcept
    {
        return otherNode == other.otherNode
        && thisChannel == other.thisChannel
        && otherChannel == other.otherChannel;
    }
        
        //==============================================================================
        struct PluginContainerProcessor::RenderSequenceFloat   : public GraphRenderSequence<float> {};
        struct PluginContainerProcessor::RenderSequenceDouble  : public GraphRenderSequence<double> {};
        
        //==============================================================================
        PluginContainerProcessor::PluginContainerProcessor(): m_bRecording(false)
    {
        m_ppfStorageBuffer = nullptr;
    }
        
        PluginContainerProcessor::~PluginContainerProcessor()
    {
        clearRenderingSequence();
        clear();
    }
        
        const String PluginContainerProcessor::getName() const
    {
        return "Audio Graph";
    }
        
        //==============================================================================
        void PluginContainerProcessor::topologyChanged()
    {
        sendChangeMessage();
        
        if (isPrepared)
            triggerAsyncUpdate();
    }
        
        void PluginContainerProcessor::clear()
    {
        if (nodes.isEmpty())
            return;
        
        nodes.clear();
        topologyChanged();
    }
        
        PluginContainerProcessor::Node* PluginContainerProcessor::getNodeForId (NodeID nodeID) const
    {
        for (auto* n : nodes)
            if (n->nodeID == nodeID)
                return n;
        
        return {};
    }
        
        PluginContainerProcessor::Node::Ptr PluginContainerProcessor::addNode (AudioProcessor* newProcessor, NodeID nodeID)
    {
        if (newProcessor == nullptr || newProcessor == this)
        {
            jassertfalse;
            return {};
        }
        
        if (nodeID == 0)
            nodeID = ++lastNodeID;
        
        for (auto* n : nodes)
        {
            if (n->getProcessor() == newProcessor || n->nodeID == nodeID)
            {
                jassertfalse; // Cannot add two copies of the same processor, or duplicate node IDs!
                return {};
            }
        }
        
        if (nodeID > lastNodeID)
            lastNodeID = nodeID;
        
        newProcessor->setPlayHead (getPlayHead());
        
        Node::Ptr n (new Node (nodeID, newProcessor));
        nodes.add (n);
        n->setParentGraph (this);
        topologyChanged();
        return n;
    }
        
        bool PluginContainerProcessor::removeNode (NodeID nodeId)
    {
        for (int i = nodes.size(); --i >= 0;)
        {
            if (nodes.getUnchecked(i)->nodeID == nodeId)
            {
                disconnectNode (nodeId);
                nodes.remove (i);
                topologyChanged();
                return true;
            }
        }
        
        return false;
    }
        
        bool PluginContainerProcessor::removeNode (Node* node)
    {
        if (node != nullptr)
            return removeNode (node->nodeID);
        
        jassertfalse;
        return false;
    }
        
        //==============================================================================
        void PluginContainerProcessor::getNodeConnections (Node& node, std::vector<Connection>& connections)
    {
        for (auto& i : node.inputs)
            connections.push_back ({ { i.otherNode->nodeID, i.otherChannel }, { node.nodeID, i.thisChannel } });
        
        for (auto& o : node.outputs)
            connections.push_back ({ { node.nodeID, o.thisChannel }, { o.otherNode->nodeID, o.otherChannel } });
    }
        
        std::vector<PluginContainerProcessor::Connection> PluginContainerProcessor::getConnections() const
    {
        std::vector<Connection> connections;
        
        for (auto& n : nodes)
            getNodeConnections (*n, connections);
        
        std::sort (connections.begin(), connections.end());
        auto last = std::unique (connections.begin(), connections.end());
        connections.erase (last, connections.end());
        
        return connections;
    }
        
        bool PluginContainerProcessor::isConnected (Node* source, int sourceChannel, Node* dest, int destChannel) const noexcept
    {
        for (auto& o : source->outputs)
            if (o.otherNode == dest && o.thisChannel == sourceChannel && o.otherChannel == destChannel)
                return true;
        
        return false;
    }
        
        bool PluginContainerProcessor::isConnected (const Connection& c) const noexcept
    {
        if (auto* source = getNodeForId (c.source.nodeID))
            if (auto* dest = getNodeForId (c.destination.nodeID))
                return isConnected (source, c.source.channelIndex,
                                    dest, c.destination.channelIndex);
        
        return false;
    }
        
        bool PluginContainerProcessor::isConnected (NodeID srcID, NodeID destID) const noexcept
    {
        if (auto* source = getNodeForId (srcID))
            if (auto* dest = getNodeForId (destID))
                for (auto& out : source->outputs)
                    if (out.otherNode == dest)
                        return true;
        
        return false;
    }
        
        bool PluginContainerProcessor::isAnInputTo (Node& src, Node& dst) const noexcept
    {
        jassert (nodes.contains (&src));
        jassert (nodes.contains (&dst));
        
        return isAnInputTo (src, dst, nodes.size());
    }
        
        bool PluginContainerProcessor::isAnInputTo (Node& src, Node& dst, int recursionCheck) const noexcept
    {
        for (auto&& i : dst.inputs)
            if (i.otherNode == &src)
                return true;
        
        if (recursionCheck > 0)
            for (auto&& i : dst.inputs)
                if (isAnInputTo (src, *i.otherNode, recursionCheck - 1))
                    return true;
        
        return false;
    }
        
        
        bool PluginContainerProcessor::canConnect (Node* source, int sourceChannel, Node* dest, int destChannel) const noexcept
    {
        bool sourceIsMIDI = sourceChannel == midiChannelIndex;
        bool destIsMIDI   = destChannel == midiChannelIndex;
        
        if (sourceChannel < 0
            || destChannel < 0
            || source == dest
            || sourceIsMIDI != destIsMIDI)
            return false;
        
        if (source == nullptr
            || (! sourceIsMIDI && sourceChannel >= source->processor->getTotalNumOutputChannels())
            || (sourceIsMIDI && ! source->processor->producesMidi()))
            return false;
        
        if (dest == nullptr
            || (! destIsMIDI && destChannel >= dest->processor->getTotalNumInputChannels())
            || (destIsMIDI && ! dest->processor->acceptsMidi()))
            return false;
        
        return ! isConnected (source, sourceChannel, dest, destChannel);
    }
        
        bool PluginContainerProcessor::canConnect (const Connection& c) const
    {
        if (auto* source = getNodeForId (c.source.nodeID))
            if (auto* dest = getNodeForId (c.destination.nodeID))
                return canConnect (source, c.source.channelIndex,
                                   dest, c.destination.channelIndex);
        
        return false;
    }
        
        bool PluginContainerProcessor::addConnection (const Connection& c)
    {
        if (auto* source = getNodeForId (c.source.nodeID))
        {
            if (auto* dest = getNodeForId (c.destination.nodeID))
            {
                auto sourceChan = c.source.channelIndex;
                auto destChan = c.destination.channelIndex;
                
                if (canConnect (source, sourceChan, dest, destChan))
                {
                    source->outputs.add ({ dest, destChan, sourceChan });
                    dest->inputs.add ({ source, sourceChan, destChan });
                    jassert (isConnected (c));
                    topologyChanged();
                    return true;
                }
            }
        }
        
        return false;
    }
        
        bool PluginContainerProcessor::removeConnection (const Connection& c)
    {
        if (auto* source = getNodeForId (c.source.nodeID))
        {
            if (auto* dest = getNodeForId (c.destination.nodeID))
            {
                auto sourceChan = c.source.channelIndex;
                auto destChan = c.destination.channelIndex;
                
                if (isConnected (source, sourceChan, dest, destChan))
                {
                    source->outputs.removeAllInstancesOf ({ dest, destChan, sourceChan });
                    dest->inputs.removeAllInstancesOf ({ source, sourceChan, destChan });
                    topologyChanged();
                    return true;
                }
            }
        }
        
        return false;
    }
        
        bool PluginContainerProcessor::disconnectNode (NodeID nodeID)
    {
        if (auto* node = getNodeForId (nodeID))
        {
            std::vector<Connection> connections;
            getNodeConnections (*node, connections);
            
            if (! connections.empty())
            {
                for (auto c : connections)
                    removeConnection (c);
                
                return true;
            }
        }
        
        return false;
    }
        
        bool PluginContainerProcessor::isLegal (Node* source, int sourceChannel, Node* dest, int destChannel) const noexcept
    {
        return (sourceChannel == midiChannelIndex ? source->processor->producesMidi()
                : isPositiveAndBelow (sourceChannel, source->processor->getTotalNumOutputChannels()))
        && (destChannel == midiChannelIndex ? dest->processor->acceptsMidi()
            : isPositiveAndBelow (destChannel, dest->processor->getTotalNumInputChannels()));
    }
        
        bool PluginContainerProcessor::isConnectionLegal (const Connection& c) const
    {
        if (auto* source = getNodeForId (c.source.nodeID))
            if (auto* dest = getNodeForId (c.destination.nodeID))
                return isLegal (source, c.source.channelIndex, dest, c.destination.channelIndex);
        
        return false;
    }
        
        bool PluginContainerProcessor::removeIllegalConnections()
    {
        bool anyRemoved = false;
        
        for (auto* node : nodes)
        {
            std::vector<Connection> connections;
            getNodeConnections (*node, connections);
            
            for (auto c : connections)
                if (! isConnectionLegal (c))
                    anyRemoved = removeConnection (c) || anyRemoved;
        }
        
        return anyRemoved;
    }
        
        //==============================================================================
        void PluginContainerProcessor::clearRenderingSequence()
    {
        ScopedPointer<RenderSequenceFloat> oldSequenceF;
        ScopedPointer<RenderSequenceDouble> oldSequenceD;
        
        {
            const ScopedLock sl (getCallbackLock());
            std::swap (renderSequenceFloat, oldSequenceF);
            std::swap (renderSequenceDouble, oldSequenceD);
        }
    }
        
        bool PluginContainerProcessor::anyNodesNeedPreparing() const noexcept
    {
        for (auto* node : nodes)
            if (! node->isPrepared)
                return true;
        
        return false;
    }
        
        void PluginContainerProcessor::buildRenderingSequence()
    {
        ScopedPointer<RenderSequenceFloat>  newSequenceF (new RenderSequenceFloat());
        ScopedPointer<RenderSequenceDouble> newSequenceD (new RenderSequenceDouble());
        
        {
            MessageManagerLock mml;
            
            RenderSequenceBuilder<RenderSequenceFloat>  builderF (*this, *newSequenceF);
            RenderSequenceBuilder<RenderSequenceDouble> builderD (*this, *newSequenceD);
        }
        
        newSequenceF->prepareBuffers (getBlockSize());
        newSequenceD->prepareBuffers (getBlockSize());
        
        if (anyNodesNeedPreparing())
        {
            {
                const ScopedLock sl (getCallbackLock());
                renderSequenceFloat.reset();
                renderSequenceDouble.reset();
            }
            
            for (auto* node : nodes)
                node->prepare (getSampleRate(), getBlockSize(), this, getProcessingPrecision());
        }
        
        const ScopedLock sl (getCallbackLock());
        
        std::swap (renderSequenceFloat, newSequenceF);
        std::swap (renderSequenceDouble, newSequenceD);
    }
        
        void PluginContainerProcessor::handleAsyncUpdate()
    {
        buildRenderingSequence();
    }
        
        //==============================================================================
        void PluginContainerProcessor::prepareToPlay (double /*sampleRate*/, int estimatedSamplesPerBlock)
    {
        if (renderSequenceFloat != nullptr)
            renderSequenceFloat->prepareBuffers (estimatedSamplesPerBlock);
        
        if (renderSequenceDouble != nullptr)
            renderSequenceDouble->prepareBuffers (estimatedSamplesPerBlock);
        
        clearRenderingSequence();
        buildRenderingSequence();
        
        isPrepared = true;

        m_ppfStorageBuffer = new float*[(unsigned long)getTotalNumInputChannels()];
        for (int iChannel = 0; iChannel < getTotalNumInputChannels(); iChannel++)
        {
            m_ppfStorageBuffer[iChannel] = new float[(unsigned long)getSampleRate()];
        }
        m_iLastLoc = 0;
        
        m_fMyFile.open(m_sOutputFilePath);
    }
        
        bool PluginContainerProcessor::supportsDoublePrecisionProcessing() const
    {
        return true;
    }
        
        void PluginContainerProcessor::releaseResources()
    {
        isPrepared = false;
        
        for (auto* n : nodes)
            n->unprepare();
        
        if (renderSequenceFloat != nullptr)
            renderSequenceFloat->releaseBuffers();
        
        if (renderSequenceDouble != nullptr)
            renderSequenceDouble->releaseBuffers();
        
        m_bRecording = false;

        for (int iChannel = 0; iChannel < getTotalNumInputChannels(); iChannel++)
        {
            delete [] m_ppfStorageBuffer[iChannel];
        }
        delete [] m_ppfStorageBuffer;
        m_ppfStorageBuffer = nullptr;
        
        m_fMyFile.close();
    }
        
        void PluginContainerProcessor::reset()
    {
        const ScopedLock sl (getCallbackLock());
        
        for (auto* n : nodes)
            n->getProcessor()->reset();
    }
        
        void PluginContainerProcessor::setNonRealtime (bool isProcessingNonRealtime) noexcept
    {
        const ScopedLock sl (getCallbackLock());
        
        AudioProcessor::setNonRealtime (isProcessingNonRealtime);
        
        for (auto* n : nodes)
            n->getProcessor()->setNonRealtime (isProcessingNonRealtime);
    }
        
        double PluginContainerProcessor::getTailLengthSeconds() const            { return 0; }
        bool PluginContainerProcessor::acceptsMidi() const                       { return true; }
        bool PluginContainerProcessor::producesMidi() const                      { return true; }
        void PluginContainerProcessor::getStateInformation (juce::MemoryBlock&)  {}
        void PluginContainerProcessor::setStateInformation (const void*, int)    {}
        
        void PluginContainerProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
    {
        const ScopedLock sl (getCallbackLock());
        
        if (renderSequenceFloat != nullptr)
            renderSequenceFloat->perform (buffer, midiMessages, getPlayHead());
        
        if (m_bRecording)
        {
            for (int iSample = 0; iSample < getBlockSize(); iSample++)
            {
                for (int iChannel = 0; iChannel < getTotalNumInputChannels(); iChannel++)
                {
                    m_ppfStorageBuffer[iChannel][iSample+m_iLastLoc] = buffer.getSample(iChannel, iSample);
                    m_fMyFile << m_ppfStorageBuffer[iChannel][iSample+m_iLastLoc] << "\t";
                }
                m_fMyFile << std::endl;
            }
            m_iLastLoc += getBlockSize();
            if (m_iLastLoc > getSampleRate())
            {
                m_bRecording = false;
            }
        }
    }
        
        void PluginContainerProcessor::processBlock (AudioBuffer<double>& buffer, MidiBuffer& midiMessages)
    {
        const ScopedLock sl (getCallbackLock());
        
        if (renderSequenceDouble != nullptr)
            renderSequenceDouble->perform (buffer, midiMessages, getPlayHead());
    }
        
        //==============================================================================
        PluginContainerProcessor::AudioGraphIOProcessor::AudioGraphIOProcessor (const IODeviceType deviceType)
        : type (deviceType)
    {
    }
        
        PluginContainerProcessor::AudioGraphIOProcessor::~AudioGraphIOProcessor()
    {
    }
        
        const String PluginContainerProcessor::AudioGraphIOProcessor::getName() const
    {
        switch (type)
        {
            case audioOutputNode:   return "Audio Output";
            case audioInputNode:    return "Audio Input";
            case midiOutputNode:    return "Midi Output";
            case midiInputNode:     return "Midi Input";
            default:                break;
        }
        
        return {};
    }
        
        void PluginContainerProcessor::AudioGraphIOProcessor::fillInPluginDescription (PluginDescription& d) const
    {
        d.name = getName();
        d.uid = d.name.hashCode();
        d.category = "I/O devices";
        d.pluginFormatName = "Internal";
        d.manufacturerName = "ROLI Ltd.";
        d.version = "1.0";
        d.isInstrument = false;
        
        d.numInputChannels = getTotalNumInputChannels();
        
        if (type == audioOutputNode && graph != nullptr)
            d.numInputChannels = graph->getTotalNumInputChannels();
        
        d.numOutputChannels = getTotalNumOutputChannels();
        
        if (type == audioInputNode && graph != nullptr)
            d.numOutputChannels = graph->getTotalNumOutputChannels();
    }
        
        void PluginContainerProcessor::AudioGraphIOProcessor::prepareToPlay (double, int)
    {
        jassert (graph != nullptr);
    }
        
        void PluginContainerProcessor::AudioGraphIOProcessor::releaseResources()
    {
    }
        
        bool PluginContainerProcessor::AudioGraphIOProcessor::supportsDoublePrecisionProcessing() const
    {
        return true;
    }
        
        template <typename FloatType, typename SequenceType>
        static void processIOBlock (PluginContainerProcessor::AudioGraphIOProcessor& io, SequenceType& sequence,
                                    AudioBuffer<FloatType>& buffer, MidiBuffer& midiMessages)
    {
        switch (io.getType())
        {
            case PluginContainerProcessor::AudioGraphIOProcessor::audioOutputNode:
            {
                auto&& currentAudioOutputBuffer = sequence.currentAudioOutputBuffer;
                
                for (int i = jmin (currentAudioOutputBuffer.getNumChannels(), buffer.getNumChannels()); --i >= 0;)
                    currentAudioOutputBuffer.addFrom (i, 0, buffer, i, 0, buffer.getNumSamples());
                
                break;
            }
                
            case PluginContainerProcessor::AudioGraphIOProcessor::audioInputNode:
            {
                auto* currentInputBuffer = sequence.currentAudioInputBuffer;
                
                for (int i = jmin (currentInputBuffer->getNumChannels(), buffer.getNumChannels()); --i >= 0;)
                    buffer.copyFrom (i, 0, *currentInputBuffer, i, 0, buffer.getNumSamples());
                
                break;
            }
                
            case PluginContainerProcessor::AudioGraphIOProcessor::midiOutputNode:
                sequence.currentMidiOutputBuffer.addEvents (midiMessages, 0, buffer.getNumSamples(), 0);
                break;
                
            case PluginContainerProcessor::AudioGraphIOProcessor::midiInputNode:
                midiMessages.addEvents (*sequence.currentMidiInputBuffer, 0, buffer.getNumSamples(), 0);
                break;
                
            default:
                break;
        }
    }
        
        void PluginContainerProcessor::AudioGraphIOProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
    {
        jassert (graph != nullptr);
        processIOBlock (*this, *graph->renderSequenceFloat, buffer, midiMessages);
    }
        
        void PluginContainerProcessor::AudioGraphIOProcessor::processBlock (AudioBuffer<double>& buffer, MidiBuffer& midiMessages)
    {
        jassert (graph != nullptr);
        processIOBlock (*this, *graph->renderSequenceDouble, buffer, midiMessages);
    }
        
        double PluginContainerProcessor::AudioGraphIOProcessor::getTailLengthSeconds() const
    {
        return 0;
    }
        
        bool PluginContainerProcessor::AudioGraphIOProcessor::acceptsMidi() const
    {
        return type == midiOutputNode;
    }
        
        bool PluginContainerProcessor::AudioGraphIOProcessor::producesMidi() const
    {
        return type == midiInputNode;
    }
        
        bool PluginContainerProcessor::AudioGraphIOProcessor::isInput() const noexcept           { return type == audioInputNode  || type == midiInputNode; }
        bool PluginContainerProcessor::AudioGraphIOProcessor::isOutput() const noexcept          { return type == audioOutputNode || type == midiOutputNode; }
        
        bool PluginContainerProcessor::AudioGraphIOProcessor::hasEditor() const                  { return false; }
        AudioProcessorEditor* PluginContainerProcessor::AudioGraphIOProcessor::createEditor()    { return nullptr; }
        
        int PluginContainerProcessor::AudioGraphIOProcessor::getNumPrograms()                    { return 0; }
        int PluginContainerProcessor::AudioGraphIOProcessor::getCurrentProgram()                 { return 0; }
        void PluginContainerProcessor::AudioGraphIOProcessor::setCurrentProgram (int)            { }
        
        const String PluginContainerProcessor::AudioGraphIOProcessor::getProgramName (int)       { return {}; }
        void PluginContainerProcessor::AudioGraphIOProcessor::changeProgramName (int, const String&) {}
        
        void PluginContainerProcessor::AudioGraphIOProcessor::getStateInformation (juce::MemoryBlock&) {}
        void PluginContainerProcessor::AudioGraphIOProcessor::setStateInformation (const void*, int) {}
        
        void PluginContainerProcessor::AudioGraphIOProcessor::setParentGraph (PluginContainerProcessor* const newGraph)
    {
        graph = newGraph;
        
        if (graph != nullptr)
        {
            setPlayConfigDetails (type == audioOutputNode ? graph->getTotalNumOutputChannels() : 0,
                                  type == audioInputNode  ? graph->getTotalNumInputChannels()  : 0,
                                  getSampleRate(),
                                  getBlockSize());
            
            updateHostDisplay();
        }
    }

        void PluginContainerProcessor::generateAudioFile(bool rec)
    {
        m_bRecording = rec;
    }
