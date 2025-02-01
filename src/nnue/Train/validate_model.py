import tensorflow as tf
from train import *

def main():
    path_data_directory = "/home/kayna/Documents/Data/"

    _, _, test_data = get_data_generators(path_data_directory, TRAIN_SPLIT, VALIDATION_SPLIT, TEST_SPLIT)
    test_data = test_data.batch(BATCH_SIZE_VALIDATION).repeat(2)

    num_positions_per_file = int(os.path.getsize(path_data_directory + "data_batch_0.bin") / (61 * 4))
    steps = int(
        math.ceil((NUM_FILES * TEST_SPLIT * num_positions_per_file) / BATCH_SIZE_VALIDATION)
    )

    loaded_model1 = tf.keras.saving.load_model(
        "./Model-SigmoidLoss-4.07",
        custom_objects={'loss_function':loss_function, "activation_function":activation_function}
    )

    loaded_model2 = tf.keras.saving.load_model(
        "./Model-SigmoidLoss(0.2 Dropout)-3.96",
        custom_objects={'loss_function':loss_function, "activation_function":activation_function}
    )
    
    evaluation1 = loaded_model1.evaluate(test_data, steps=steps)
    evaluation2 = loaded_model2.evaluate(test_data, steps=steps)

    print("No Dropout model evaluation: ")
    print(evaluation1)
    
    print("Dropout model evaluation: ")
    print(evaluation2)

if __name__ == "__main__":
    main()
