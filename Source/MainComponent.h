#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/

using namespace juce;

class TaskQueueItem final : public TreeViewItem,
                            private ValueTree::Listener
{
public:
    TaskQueueItem (const ValueTree& v, UndoManager& um)
        : tree (v), undoManager (um)
    {
        tree.addListener (this);
    }

    String getUniqueName() const override
    {
        return tree["name"].toString();
    }

    bool mightContainSubItems() override
    {
        return tree.getNumChildren() > 0;
    }

    void paintItem (Graphics& g, int width, int height) override
    {
        if (isSelected())
        {
            g.fillAll(Colours::teal);
        }
        g.setColour(Colours::black);
        
        g.setFont (15.0f);

        g.drawText (tree["name"].toString(),
                    4, 0, width - 4, height,
                    Justification::centredLeft, true);
    }

    void itemOpennessChanged (bool isNowOpen) override
    {
        if (isNowOpen && getNumSubItems() == 0)
            refreshSubItems();
        else
            clearSubItems();
    }

    var getDragSourceDescription() override
    {
        return "Drag Demo";
    }

    bool isInterestedInDragSource (const DragAndDropTarget::SourceDetails& dragSourceDetails) override
    {
        return dragSourceDetails.description == "Drag Demo";
    }

    void itemDropped (const DragAndDropTarget::SourceDetails&, int insertIndex) override
    {
        OwnedArray<ValueTree> selectedTrees;
        getSelectedTreeViewItems (*getOwnerView(), selectedTrees);

        moveItems (*getOwnerView(), selectedTrees, tree, insertIndex, undoManager);
    }

    static void moveItems (TreeView& treeView, const OwnedArray<ValueTree>& items,
                           ValueTree newParent, int insertIndex, UndoManager& undoManager)
    {
        if (items.size() > 0)
        {
            std::unique_ptr<XmlElement> oldOpenness (treeView.getOpennessState (false));

            for (auto* v : items)
            {
                if (v->getParent().isValid() && newParent != *v && ! newParent.isAChildOf (*v))
                {
                    if (v->getParent() == newParent && newParent.indexOf (*v) < insertIndex)
                        --insertIndex;

                    v->getParent().removeChild (*v, &undoManager);
                    newParent.addChild (*v, insertIndex, &undoManager);
                }
            }

            if (oldOpenness != nullptr)
                treeView.restoreOpennessState (*oldOpenness, false);
        }
    }

    static void getSelectedTreeViewItems (TreeView& treeView, OwnedArray<ValueTree>& items)
    {
        auto numSelected = treeView.getNumSelectedItems();

        for (int i = 0; i < numSelected; ++i)
            if (auto* vti = dynamic_cast<TaskQueueItem*> (treeView.getSelectedItem (i)))
                items.add (new ValueTree (vti->tree));
    }

private:
    ValueTree tree;
    UndoManager& undoManager;

    void refreshSubItems()
    {
        clearSubItems();

        for (int i = 0; i < tree.getNumChildren(); ++i)
            addSubItem (new TaskQueueItem (tree.getChild (i), undoManager));
    }

    void valueTreePropertyChanged (ValueTree&, const Identifier&) override
    {
        repaintItem();
    }

    void valueTreeChildAdded (ValueTree& parentTree, ValueTree&) override         { treeChildrenChanged (parentTree); }
    void valueTreeChildRemoved (ValueTree& parentTree, ValueTree&, int) override  { treeChildrenChanged (parentTree); }
    void valueTreeChildOrderChanged (ValueTree& parentTree, int, int) override    { treeChildrenChanged (parentTree); }
    void valueTreeParentChanged (ValueTree&) override {}

    void treeChildrenChanged (const ValueTree& parentTree)
    {
        if (parentTree == tree)
        {
            refreshSubItems();
            treeHasChanged();
            setOpen (true);
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TaskQueueItem)
};

//==============================================================================
class TaskQueueContainer final : public Component,
                             public DragAndDropContainer,
                             private Timer
{
public:
    TaskQueueContainer()
    {
        addAndMakeVisible (tree);

        tree.setTitle ("ValueTree");
        tree.setDefaultOpenness (true);
        tree.setMultiSelectEnabled (true);
        rootItem.reset (new TaskQueueItem (createRootValueTree(), undoManager));
        tree.setRootItem (rootItem.get());

        startTimer (500);

        setSize (500, 500);
    }

    ~TaskQueueContainer() override
    {
        tree.setRootItem (nullptr);
    }

    void paint (Graphics& g) override
    {
        g.fillAll(Colours::white);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (8);

        tree.setBounds (r);
    }

    ValueTree createRootValueTree()
    {
        auto vt = createTree("Task Queue Tree");
        return vt;
    }

private:
    ValueTree createTree (const String& desc)
    {
        ValueTree t ("Item");
        t.setProperty ("name", desc, nullptr);
        return t;
    }
    
    TreeView tree;

    std::unique_ptr<TaskQueueItem> rootItem;
    UndoManager undoManager;

    void timerCallback() override
    {
        undoManager.beginNewTransaction();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TaskQueueContainer)
};


class MainComponent  : public juce::Component
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    //==============================================================================
    // Your private member variables go here...
    
    TaskQueueContainer taskQueueContainer;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
