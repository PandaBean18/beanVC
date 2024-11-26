def convert_string_to_int(str):
    nums = "0123456789"
    val = 0
    for c in str:
        i = 0
        while (i<len(nums)):
            if (nums[i] == c):
                val *= 10
                val += i
                break
            i += 1

    return val

def runAutomatedTests():
    assert convert_string_to_int("123")==123, "Failed at input '123'" 
    assert convert_string_to_int("031")==31, "Failed at input '031'"
    assert convert_string_to_int("1023")==1023, "Failed at input '1023'"
    assert convert_string_to_int("0010")==10, "Failed at input '0010'"
    assert convert_string_to_int("0000")==0, "Failed at input '0000'"
    print("All tests ran successfully.")

def runManualTests():
    print("Input exit to stop.")
    while (1):
        s = input("string: ")

        if (s == "exit"):
            break

        v = convert_string_to_int(s)
        print("The function outputted: ", v)

runAutomatedTests()
runManualTests()
