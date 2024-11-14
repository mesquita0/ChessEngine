import numpy as np
import tensorflow as tf
from tensorflow.keras import Input
from tensorflow.keras.layers import concatenate, Dense, Dropout
from tensorflow.keras.models import Model
from keras.constraints import Constraint
import keras.backend

import math
import os

NUM_THREADS = 6
EPOCHS = 50
BATCH_SIZE = 512
NUM_NEURONS_INPUT_LAYER_PER_SIDE = 64 * 64 * 10
TRAIN_SPLIT = 0.7
VALIDATION_SPLIT = 0.15
TEST_SPLIT = 0.15

NUM_FILES = 50
DATA_ELEMENT_SIZE = 61 * 4
BATCH_SIZE_VALIDATION = 100_000

SCALING_QUANT_ACC = 127
SCALING_QUANT_OUTPUT = 150
SCALING_QUANT_WEIGHTS = 64 # needs to be a power of two

MIN_WEIGHT_LIN_L = -128 / SCALING_QUANT_WEIGHTS
MAX_WEIGHT_LIN_L =  127 / SCALING_QUANT_WEIGHTS

MIN_WEIGHT_OUT_L = -128 * SCALING_QUANT_ACC / (SCALING_QUANT_WEIGHTS * SCALING_QUANT_OUTPUT)
MAX_WEIGHT_OUT_L =  127 * SCALING_QUANT_ACC / (SCALING_QUANT_WEIGHTS * SCALING_QUANT_OUTPUT)

USE_SIGMOID_ACTIVATION = False


def activation_function(x): 
    if (USE_SIGMOID_ACTIVATION):
        return tf.sigmoid(tf.multiply(x, tf.constant([4], dtype=tf.float32))) # sigmoid(4x)
    
    return tf.math.minimum(tf.maximum(x, 0), 1) # ClippedReLu


MAX_ERROR = 100
SCALING_SIGMOID = 1.0/256
def loss_function(y_true, y_pred):
    wdl_eval_true = tf.sigmoid(tf.multiply(tf.cast(y_true, tf.float32), SCALING_SIGMOID))

    # (y_pred * scaling_output) * scaling_sigmoid
    wdl_eval_pred = tf.sigmoid(tf.multiply(tf.multiply(tf.cast(y_pred, tf.float32), SCALING_QUANT_OUTPUT), SCALING_SIGMOID))

    loss_eval = tf.math.pow(tf.subtract(wdl_eval_pred, wdl_eval_true), tf.constant([2], dtype=tf.float32))

    return tf.multiply(loss_eval, MAX_ERROR)


def main():
    path_data_directory = "/home/kayna/Documents/Data/"
    num_positions_per_file = int(os.path.getsize(path_data_directory + "data_batch_0.bin") / (61 * 4))

    training_data, validation_data, test_data = get_data_generators(path_data_directory, TRAIN_SPLIT, VALIDATION_SPLIT, TEST_SPLIT)

    steps_per_epoch = int(
        math.ceil((NUM_FILES * TRAIN_SPLIT * num_positions_per_file) / BATCH_SIZE)
    )

    model = get_model()
    
    tensorboard_callback = tf.keras.callbacks.TensorBoard(histogram_freq=1)
    early_stopping_callback = tf.keras.callbacks.EarlyStopping(patience=2)
    checkpoint_callback = tf.keras.callbacks.ModelCheckpoint(
        "Model-{epoch:02d}-{val_loss:0.2f}", 
        monitor="val_loss", mode="min", 
        save_best_only=True
    )
    callbacks = [tensorboard_callback, early_stopping_callback, checkpoint_callback]

    model.fit(
        training_data, 
        validation_data=validation_data, 
        steps_per_epoch=steps_per_epoch,
        epochs=EPOCHS,
        callbacks=callbacks
    )


def get_model():
    input_sm = Input(shape=(NUM_NEURONS_INPUT_LAYER_PER_SIDE,), sparse=True, name="side_to_move")
    input_snm = Input(shape=(NUM_NEURONS_INPUT_LAYER_PER_SIDE,), sparse=True, name="side_not_to_move")
    input_process_layer = Dense(256, activation=activation_function)
    input_process_layer_side_move = input_process_layer(input_sm)
    input_process_layer_side_not_move = input_process_layer(input_snm)

    hidden1 = concatenate([input_process_layer_side_move, input_process_layer_side_not_move])
    dropout1 = Dropout(0.2)(hidden1)

    hidden2 = Dense(
        32, 
        activation=activation_function, 
        kernel_constraint=Between(MIN_WEIGHT_LIN_L, MAX_WEIGHT_LIN_L)
    )(dropout1)
    dropout2 = Dropout(0.1)(hidden2)

    hidden3 = Dense(
        32, 
        activation=activation_function, 
        kernel_constraint=Between(MIN_WEIGHT_LIN_L, MAX_WEIGHT_LIN_L)
    )(dropout2)
    
    output = Dense(
        1, 
        kernel_constraint=Between(MIN_WEIGHT_OUT_L, MAX_WEIGHT_OUT_L)
    )(hidden3)

    model = Model(inputs=[input_sm, input_snm], outputs=output)
    model.compile(
        optimizer="adam",
        loss=loss_function,
        metrics=["mean_absolute_error", "mean_squared_error"]
    )

    return model


def get_data_generators(path_data_directory, training_split, validation_split, test_split):

    indices = list(range(NUM_FILES))
    last_index_training = math.ceil(training_split*NUM_FILES)
    training_indices = indices[:last_index_training]
    last_index_validation = last_index_training + math.ceil(validation_split*NUM_FILES)
    validation_indices = indices[last_index_training:last_index_validation]
    test_indices = indices[last_index_validation:]

    training_data = tf.data.FixedLengthRecordDataset(
        filenames=[f"{path_data_directory}data_batch_{i}.bin" for i in training_indices],
        record_bytes=DATA_ELEMENT_SIZE
    ).map(map_func, num_parallel_calls=NUM_THREADS).batch(BATCH_SIZE).cache("./cache/training").shuffle(1000).repeat(EPOCHS).prefetch(tf.data.AUTOTUNE)
    
    validation_data = tf.data.FixedLengthRecordDataset(
        filenames=[f"{path_data_directory}data_batch_{i}.bin" for i in validation_indices],
        record_bytes=DATA_ELEMENT_SIZE
    ).map(map_func).batch(BATCH_SIZE_VALIDATION).cache("./cache/validation").prefetch(tf.data.AUTOTUNE)
    
    test_data = tf.data.FixedLengthRecordDataset(
        filenames=[f"{path_data_directory}data_batch_{i}.bin" for i in test_indices],
        record_bytes=DATA_ELEMENT_SIZE
    ).map(map_func)

    return training_data, validation_data, test_data


def map_func(binary_data):
    'Maps data to sparse tensor.'

    data = tf.io.decode_raw(binary_data, tf.int32)

    # Remove duplicate zeros
    dt_sm, _ = tf.unique(data[:30])
    dt_snm, _ = tf.unique(data[30:-1])

    evaluation = data[-1]

    # Convert data to sparse tensors
    x_sm = to_sparse_tensor(
        tf.reshape(dt_sm, (tf.shape(dt_sm)[0], 1))
    )
    
    x_snm = to_sparse_tensor(
        tf.reshape(dt_snm, (tf.shape(dt_snm)[0], 1))
    )

    return {"side_to_move": x_sm, "side_not_to_move": x_snm}, evaluation


def to_sparse_tensor(x):
    return tf.sparse.SparseTensor(
        indices=tf.cast(x, tf.int64),
        values=tf.ones(tf.shape(x)[0], dtype=tf.int32),
        dense_shape=(NUM_NEURONS_INPUT_LAYER_PER_SIDE,)
    )


class Between(Constraint):
    def __init__(self, min_value, max_value):
        self.min_value = min_value
        self.max_value = max_value

    def __call__(self, w):        
        return keras.backend.clip(w, self.min_value, self.max_value)

    def get_config(self):
        return {'min_value': self.min_value,
                'max_value': self.max_value}
    


if __name__ == "__main__":
    main()
