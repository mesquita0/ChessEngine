import tensorflow as tf
from train import *

def main():

    loaded_model = tf.keras.saving.load_model(
        "./Model-CReLU-activation-3.96",
        custom_objects={'loss_function':loss_function, "activation_function":activation_function}
    )

    vec_acc = np.vectorize(lambda x: np.round(SCALING_QUANT_ACC * x), otypes=[np.int16])
    vec_weights_lin = np.vectorize(lambda x: np.round(SCALING_QUANT_WEIGHTS * x), otypes=[np.int8])
    vec_biases_lin  = np.vectorize(lambda x: np.round(SCALING_QUANT_WEIGHTS * SCALING_QUANT_ACC * x), otypes=[np.int32])
    vec_weights_out = np.vectorize(lambda x: np.round((SCALING_QUANT_WEIGHTS * SCALING_QUANT_OUTPUT) / SCALING_QUANT_ACC * x), otypes=[np.int8])
    vec_biases_out  = np.vectorize(lambda x: np.round(SCALING_QUANT_WEIGHTS * SCALING_QUANT_OUTPUT * x), otypes=[np.int32])

    # First dense layer (accumulator)
    vec_acc(loaded_model.layers[2].get_weights()[0]).tofile("./Weights/accw.bin") # Weights
    vec_acc(loaded_model.layers[2].get_weights()[1]).tofile("./Weights/accb.bin") # Biases

    # Second dense layer (first linear layer)
    vec_weights_lin(loaded_model.layers[5].get_weights()[0]).transpose().tofile("./Weights/lin1w.bin")
    vec_biases_lin(loaded_model.layers[5].get_weights()[1]).tofile("./Weights/lin1b.bin")

    # Third dense layer (second linear layer)
    vec_weights_lin(loaded_model.layers[7].get_weights()[0]).transpose().tofile("./Weights/lin2w.bin")
    vec_biases_lin(loaded_model.layers[7].get_weights()[1]).tofile("./Weights/lin2b.bin")

    # Fourth dense layer (third linear layer)
    vec_weights_out(loaded_model.layers[8].get_weights()[0]).transpose().tofile("./Weights/lin3w.bin") # Weights
    vec_biases_out(loaded_model.layers[8].get_weights()[1]).tofile("./Weights/lin3b.bin") # Biases

    print(vec_weights_lin(loaded_model.layers[5].get_weights()[0]).transpose())

if __name__ == "__main__":
    main()
