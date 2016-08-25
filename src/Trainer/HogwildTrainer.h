#ifndef _HOGWILD_TRAINER_
#define _HOGWILD_TRAINER_

template<class GRADIENT_CLASS>
class HogwildTrainer : public Trainer<GRADIENT_CLASS> {
public:
    HogwildTrainer() {}
    ~HogwildTrainer() {}

    void Train(Model *model, const std::vector<Datapoint *> & datapoints, Updater<GRADIENT_CLASS> *updater) override {
	// Partition.
	BasicPartitioner partitioner;
	Timer partition_timer;
	DatapointPartitions partitions = partitioner.Partition(datapoints, FLAGS_n_threads);
	if (FLAGS_print_partition_time) {
	    this->PrintPartitionTime(partition_timer);
	}

	model->SetUpWithPartitions(partitions);

	// Train.
	Timer gradient_timer;
	for (int epoch = 0; epoch < FLAGS_n_epochs; epoch++) {
	    if (FLAGS_print_loss_per_epoch) {
		this->PrintTimeLoss(gradient_timer, model, datapoints);
	    }

	    updater->EpochBegin();

#pragma omp parallel for schedule(static, 1)
	    for (int thread = 0; thread < FLAGS_n_threads; thread++) {
		int batch = 0; // Hogwild only has 1 batch.
		for (int index = 0; index < partitions.NumDatapointsInBatch(thread, batch); index++) {
		    updater->UpdateWrapper(model, partitions.GetDatapoint(thread, batch, index), thread);
		}
	    }
	    updater->EpochFinish();
	}
    }
};

#endif
