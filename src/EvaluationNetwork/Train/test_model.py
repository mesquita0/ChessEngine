import numpy as np
import tensorflow as tf
from train import *

def main():
    path_data_directory = "/home/kayna/Documents/Data/"
    dt = np.dtype([('side_to_move', 'i4', 30), ('side_not_to_move', 'i4', 30), ('evaluation', 'i4', 1)])
    position = np.fromfile(path_data_directory + "data_batch_44.bin", dtype=dt, count=1)[0]

    dt_sm = np.unique(position['side_to_move'])
    dt_sm = dt_sm.reshape(dt_sm.shape[0], 1)
    dt_snm = np.unique(position['side_not_to_move'])
    dt_snm = dt_snm.reshape(dt_snm.shape[0], 1)

    loaded_model1 = tf.keras.saving.load_model(
        "./Model-SigmoidLoss-4.07",
        custom_objects={'loss_function':loss_function, "activation_function":activation_function}
    )
    loaded_model2 = tf.keras.saving.load_model(
        "./Model-CReLU-activation-3.96",
        custom_objects={'loss_function':loss_function, "activation_function":activation_function}
    )
    
    inp = tf.data.Dataset.from_tensor_slices([[dt_sm, dt_snm]]).map(lambda x: [to_sparse_tensor(x[0]), to_sparse_tensor(x[1])]).batch(1)
    
    prediction = loaded_model1.predict([*inp])
    print("No Dropout model evaluation: ")
    print(prediction * SCALING_QUANT_OUTPUT)
    print(position['evaluation'])
    print(loss_function(position['evaluation'], prediction * SCALING_QUANT_OUTPUT).numpy())
    
    prediction = loaded_model2.predict([*inp])
    print("CReLU model evaluation: ")
    print(prediction)
    print(position['evaluation'])
    print(loss_function(position['evaluation'], prediction).numpy())

if __name__ == "__main__":
    main()
