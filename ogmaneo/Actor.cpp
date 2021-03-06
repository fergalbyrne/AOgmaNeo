// ----------------------------------------------------------------------------
//  AOgmaNeo
//  Copyright(c) 2020 Ogma Intelligent Systems Corp. All rights reserved.
//
//  This copy of AOgmaNeo is licensed to you under the terms described
//  in the AOGMANEO_LICENSE.md file included in this distribution.
// ----------------------------------------------------------------------------

#include "Actor.h"

using namespace ogmaneo;

void Actor::forward(
    const Int2 &pos,
    const Array<const IntBuffer*> &inputCs
) {
    int hiddenColumnIndex = address2(pos, Int2(hiddenSize.x, hiddenSize.y));

    // --- Value ---

    float value = 0.0f;
    int count = 0;

    // For each visible layer
    for (int vli = 0; vli < visibleLayers.size(); vli++) {
        VisibleLayer &vl = visibleLayers[vli];
        const VisibleLayerDesc &vld = visibleLayerDescs[vli];

        value += vl.valueWeights.multiplyOHVs(*inputCs[vli], hiddenColumnIndex, vld.size.z);
        count += vl.valueWeights.count(hiddenColumnIndex) / vld.size.z;
    }

    hiddenValues[hiddenColumnIndex] = value / count;

    // --- Action ---

    Array<float> activations(hiddenSize.z);
    float maxActivation = -999999.0f;

    for (int hc = 0; hc < hiddenSize.z; hc++) {
        int hiddenIndex = address3(Int3(pos.x, pos.y, hc), hiddenSize);

        float sum = 0.0f;

        // For each visible layer
        for (int vli = 0; vli < visibleLayers.size(); vli++) {
            VisibleLayer &vl = visibleLayers[vli];
            const VisibleLayerDesc &vld = visibleLayerDescs[vli];

            sum += vl.actionWeights.multiplyOHVs(*inputCs[vli], hiddenIndex, vld.size.z);
        }

        sum /= count;

        activations[hc] = sum;

        maxActivation = max(maxActivation, sum);
    }

    float total = 0.0f;

    for (int hc = 0; hc < hiddenSize.z; hc++) {
        activations[hc] = expf(activations[hc] - maxActivation);
        
        total += activations[hc];
    }

    float cusp = randf() * total;

    int selectIndex = 0;
    float sumSoFar = 0.0f;

    for (int hc = 0; hc < hiddenSize.z; hc++) {
        sumSoFar += activations[hc];

        if (sumSoFar >= cusp) {
            selectIndex = hc;

            break;
        }
    }
    
    hiddenCs[hiddenColumnIndex] = selectIndex;
}

void Actor::learn(
    const Int2 &pos,
    const Array<const IntBuffer*> &inputCsPrev,
    const IntBuffer* hiddenTargetCsPrev,
    const FloatBuffer* hiddenValuesPrev,
    float q,
    float g,
    bool mimic
) {
    int hiddenColumnIndex = address2(pos, Int2(hiddenSize.x, hiddenSize.y));

    // --- Value Prev ---

    float newValue = q + g * hiddenValues[hiddenColumnIndex];

    float value = 0.0f;
    int count = 0;

    // For each visible layer
    for (int vli = 0; vli < visibleLayers.size(); vli++) {
        VisibleLayer &vl = visibleLayers[vli];
        const VisibleLayerDesc &vld = visibleLayerDescs[vli];

        value += vl.valueWeights.multiplyOHVs(*inputCsPrev[vli], hiddenColumnIndex, vld.size.z);
        count += vl.valueWeights.count(hiddenColumnIndex) / vld.size.z;
    }

    value /= count;

    float tdErrorValue = newValue - value;
    
    float deltaValue = alpha * tdErrorValue;

    // For each visible layer
    for (int vli = 0; vli < visibleLayers.size(); vli++) {
        VisibleLayer &vl = visibleLayers[vli];
        const VisibleLayerDesc &vld = visibleLayerDescs[vli];

        vl.valueWeights.deltaOHVs(*inputCsPrev[vli], deltaValue, hiddenColumnIndex, vld.size.z);
    }

    // --- Action ---

    float tdErrorAction = newValue - (*hiddenValuesPrev)[hiddenColumnIndex];

    int targetC = (*hiddenTargetCsPrev)[hiddenColumnIndex];

    Array<float> activations(hiddenSize.z);
    float maxActivation = -999999.0f;

    for (int hc = 0; hc < hiddenSize.z; hc++) {
        int hiddenIndex = address3(Int3(pos.x, pos.y, hc), hiddenSize);

        float sum = 0.0f;

        // For each visible layer
        for (int vli = 0; vli < visibleLayers.size(); vli++) {
            VisibleLayer &vl = visibleLayers[vli];
            const VisibleLayerDesc &vld = visibleLayerDescs[vli];

            sum += vl.actionWeights.multiplyOHVs(*inputCsPrev[vli], hiddenIndex, vld.size.z);
        }

        sum /= count;

        activations[hc] = sum;

        maxActivation = max(maxActivation, sum);
    }

    float total = 0.0f;

    for (int hc = 0; hc < hiddenSize.z; hc++) {
        activations[hc] = expf(activations[hc] - maxActivation);
        
        total += activations[hc];
    }
    
    for (int hc = 0; hc < hiddenSize.z; hc++) {
        int hiddenIndex = address3(Int3(pos.x, pos.y, hc), hiddenSize);

        float deltaAction = (mimic ? beta : (tdErrorAction > 0.0f ? beta : -beta)) * ((hc == targetC ? 1.0f : 0.0f) - activations[hc] / max(0.0001f, total));

        // For each visible layer
        for (int vli = 0; vli < visibleLayers.size(); vli++) {
            VisibleLayer &vl = visibleLayers[vli];
            const VisibleLayerDesc &vld = visibleLayerDescs[vli];

            vl.actionWeights.deltaOHVs(*inputCsPrev[vli], deltaAction, hiddenIndex, vld.size.z);
        }
    }
}

void Actor::initRandom(
    const Int3 &hiddenSize,
    int historyCapacity,
    const Array<VisibleLayerDesc> &visibleLayerDescs
) {
    this->visibleLayerDescs = visibleLayerDescs;

    this->hiddenSize = hiddenSize;

    visibleLayers.resize(visibleLayerDescs.size());

    // Pre-compute dimensions
    int numHiddenColumns = hiddenSize.x * hiddenSize.y;

    // Create layers
    for (int vli = 0; vli < visibleLayers.size(); vli++) {
        VisibleLayer &vl = visibleLayers[vli];
        VisibleLayerDesc &vld = this->visibleLayerDescs[vli];

        // Create weight matrix for this visible layer and initialize randomly
        initSMLocalRF(vld.size, Int3(hiddenSize.x, hiddenSize.y, 1), vld.radius, vl.valueWeights);
        initSMLocalRF(vld.size, hiddenSize, vld.radius, vl.actionWeights);

        for (int i = 0; i < vl.valueWeights.nonZeroValues.size(); i++)
            vl.valueWeights.nonZeroValues[i] = 0.0f;

        for (int i = 0; i < vl.actionWeights.nonZeroValues.size(); i++)
            vl.actionWeights.nonZeroValues[i] = randf(-0.01f, 0.01f);
    }

    hiddenCs = IntBuffer(numHiddenColumns, 0);

    hiddenValues = FloatBuffer(numHiddenColumns, 0.0f);

    // Create (pre-allocated) history samples
    historySize = 0;
    historySamples.resize(historyCapacity);

    for (int i = 0; i < historySamples.size(); i++) {
        historySamples[i].inputCs.resize(visibleLayers.size());

        for (int vli = 0; vli < visibleLayers.size(); vli++) {
            VisibleLayerDesc &vld = this->visibleLayerDescs[vli];

            int numVisibleColumns = vld.size.x * vld.size.y;

            historySamples[i].inputCs[vli] = IntBuffer(numVisibleColumns);
        }

        historySamples[i].hiddenTargetCsPrev = IntBuffer(numHiddenColumns);

        historySamples[i].hiddenValuesPrev = FloatBuffer(numHiddenColumns);
    }
}

void Actor::step(
    const Array<const IntBuffer*> &inputCs,
    const IntBuffer* hiddenTargetCsPrev,
    float reward,
    bool learnEnabled,
    bool mimic
) {
    // Forward kernel
    for (int x = 0; x < hiddenSize.x; x++)
        for (int y = 0; y < hiddenSize.y; y++)
            forward(Int2(x, y), inputCs);

    historySamples.pushFront();

    // If not at cap, increment
    if (historySize < historySamples.size())
        historySize++;
    
    // Add new sample
    {
        HistorySample &s = historySamples[0];

        for (int vli = 0; vli < visibleLayers.size(); vli++)
            copyInt(inputCs[vli], &s.inputCs[vli]);

        // Copy hidden Cs
        copyInt(hiddenTargetCsPrev, &s.hiddenTargetCsPrev);

        // Copy hidden values
        copyFloat(&hiddenValues, &s.hiddenValuesPrev);

        s.reward = reward;
    }

    // Learn (if have sufficient samples)
    if (learnEnabled && historySize > minSteps + 1) {
        for (int it = 0; it < historyIters; it++) {
            int historyIndex = rand() % (historySize - 1 - minSteps) + minSteps;

            const HistorySample &sPrev = historySamples[historyIndex + 1];
            const HistorySample &s = historySamples[historyIndex];

            // Compute (partial) values, rest is completed in the kernel
            float q = 0.0f;
            float g = 1.0f;

            for (int t = historyIndex; t >= 0; t--) {
                q += historySamples[t].reward * g;

                g *= gamma;
            }

            for (int x = 0; x < hiddenSize.x; x++)
                for (int y = 0; y < hiddenSize.y; y++)
                    learn(Int2(x, y), constGet(sPrev.inputCs), &s.hiddenTargetCsPrev, &sPrev.hiddenValuesPrev, q, g, mimic);
        }
    }
}