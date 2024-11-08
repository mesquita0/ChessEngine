import tensorflow as tf
from train import *

def main():

    loaded_model = tf.keras.saving.load_model(
        "./Model-CReLU-activation-3.96",
        custom_objects={'loss_function':loss_function, "activation_function":activation_function}
    )

    vec_int8  = np.vectorize(lambda x: np.round(scalling_quant * x), otypes=[np.int8])
    vec_int16 = np.vectorize(lambda x: np.round(scalling_quant * x), otypes=[np.int16])
    vec_int32 = np.vectorize(lambda x: np.round(scalling_quant * x), otypes=[np.int32])

    # First dense layer (accumulator)
    vec_int16(loaded_model.layers[2].get_weights()[0]).tofile("./Weights/accw.bin") # Weights
    vec_int16(loaded_model.layers[2].get_weights()[1]).tofile("./Weights/accb.bin") # Biases

    # Second dense layer (first linear layer)
    vec_int8(loaded_model.layers[5].get_weights()[0]).transpose().tofile("./Weights/lin1w.bin") # Weights
    vec_int32(loaded_model.layers[5].get_weights()[1]).tofile("./Weights/lin1b.bin") # Biases

    # Third dense layer (second linear layer)
    vec_int8(loaded_model.layers[7].get_weights()[0]).transpose().tofile("./Weights/lin2w.bin") # Weights
    vec_int32(loaded_model.layers[7].get_weights()[1]).tofile("./Weights/lin2b.bin") # Biases

    # Fourth dense layer (third linear layer)
    vec_int8(loaded_model.layers[8].get_weights()[0]).transpose().tofile("./Weights/lin3w.bin") # Weights
    vec_int32(loaded_model.layers[8].get_weights()[1]).tofile("./Weights/lin3b.bin") # Biases

    print(vec_int8(loaded_model.layers[5].get_weights()[0]).transpose())

if __name__ == "__main__":
    main()
